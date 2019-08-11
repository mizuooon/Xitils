#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include <thread>
#include <memory>

#define XITILS_APP(x) CINDER_APP(x, RendererGl, [&](ci::app::App::Settings *settings){ settings->setConsoleWindowEnabled(); })

namespace Xitils::App {

	template<typename T> class XApp : public ci::app::App {
	public:
		void setup() override final;
		void draw() override final;
		void cleanup() override final;

		void mainLoop();

	protected:
		virtual void onSetup(T* frameData) {}
		virtual void onCleanup(T* frameData) {}
		virtual void onUpdate(T* frameData) {}
		virtual void onDraw(const T& frameData) {}

	private:
		T frameData;
		T frameDataBuffer;
		std::mutex mtx;

		std::shared_ptr<std::thread> mainLoopThread;
		bool threadClosing = false;
	};

	template<typename T> void XApp<T>::setup() {
		
		onSetup(&frameData);

		mainLoopThread = std::make_shared<std::thread>([&] {
			this->mainLoop();
			});
	}

	template<typename T> void XApp<T>::draw() {
		std::lock_guard lock(mtx);
		onDraw(frameDataBuffer);
	}

	template<typename T> void XApp<T>::cleanup() {
		if (mainLoopThread) {
			threadClosing = true;
			mainLoopThread->join();
		}
		onCleanup(&frameData);
	}

	template<typename T> void XApp<T>::mainLoop() {
		float elapsed = 0.0f;

		while (true) {
			onUpdate(&frameData);

			{
				std::lock_guard lock(mtx);
				frameDataBuffer = frameData;
			}

			std::this_thread::yield();

			if (threadClosing) { break; }
		}
	}

}