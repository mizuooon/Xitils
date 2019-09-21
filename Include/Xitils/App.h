#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include <thread>
#include <memory>

#define XITILS_APP(x) CINDER_APP(x, RendererGl, [&](ci::app::App::Settings *settings){ settings->setConsoleWindowEnabled(); })

namespace xitils::app {

	template<typename T, typename U> class XApp : public ci::app::App {
	public:
		void setup() override final;
		void cleanup() override final;

		void mainLoop();
		void draw() override final;

		void keyDown(ci::app::KeyEvent e) override final;

	protected:
		virtual void onSetup(T* frameData, U* uiFrameData) {}
		virtual void onCleanup(T* frameData, U* uiFrameData) {}
		virtual void onUpdate(T& frameData, const U& uiFrameData) {}
		virtual void onDraw(const T& frameData, U& uiFrameData) {}

	private:
		T frameData;
		T frameDataBuffer;

		U uiFrameData;
		U uiFrameDataBuffer;

		std::mutex mtx;

		std::shared_ptr<std::thread> mainLoopThread;
		bool threadClosing = false;
	};

	template<typename T, typename U> void XApp<T,U>::setup() {
		
		onSetup(&frameData, &uiFrameData);

		mainLoopThread = std::make_shared<std::thread>([&] {
			this->mainLoop();
			});
	}

	template<typename T, typename U> void XApp<T, U>::cleanup() {
		if (mainLoopThread) {
			threadClosing = true;
			mainLoopThread->join();
		}
		onCleanup(&frameData, &uiFrameData);
	}

	template<typename T, typename U> void XApp<T,U>::mainLoop() {
		float elapsed = 0.0f;

		while (true) {
			onUpdate(frameData, uiFrameDataBuffer);

			{
				std::lock_guard lock(mtx);
				frameDataBuffer = frameData;
			}

			std::this_thread::yield();

			if (threadClosing) { break; }
		}
	}

	template<typename T, typename U> void XApp<T, U>::draw() {
		std::lock_guard lock(mtx);
		onDraw(frameDataBuffer, uiFrameData);
		uiFrameDataBuffer = uiFrameData;
	}

	template<typename T, typename U> void XApp<T, U>::keyDown(ci::app::KeyEvent e) {
		std::lock_guard lock(mtx);
		if (e.getCode() == ci::app::KeyEvent::KEY_F1) {
			writeImage("./screesnshot.png", copyWindowSurface());
		}
	}

}