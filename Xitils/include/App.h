#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include <thread>
#include <memory>

#define XITILS_APP(x) CINDER_APP(Xitils::App::_XApp<x>, RendererGl)

namespace Xitils::App {

	template<typename T> class UpdateThread {
	public:
		virtual void onSetup() {};
		virtual void onUpdate(T* frameData) {};
		virtual void onCleanup() {};
	};

	template<typename T> class DrawThread {
	public:
		virtual void onSetup() {};
		virtual void onDraw(const T& frameData) {};
		virtual void onCleanup() {};
	};

	template<typename T> class XApp : public ci::app::App {
	public:
		void setup() override;
		void draw() override;
		void cleanup() override;

		void mainLoop();

	protected:
		virtual void onSetup() {};
		virtual void onCleanup() {};
		T frameData;

		virtual std::shared_ptr<UpdateThread<T>> createUpdateThread() = 0;
		virtual std::shared_ptr<DrawThread<T>> createDrawThread() = 0;

	private:
		std::shared_ptr<UpdateThread<T>> updateThread;
		std::shared_ptr<DrawThread<T>> drawThread;

		T frameDataBuffer;
		std::mutex mtx;

		std::shared_ptr<std::thread> mainLoopThread;
		bool threadClosing = false;
	};

	template<typename T>class _XApp : public T {
	protected:
		std::shared_ptr<UpdateThread<decltype(T::frameData)>> createUpdateThread() override {
			return std::make_shared<T::UpdateThread>();
		}

		std::shared_ptr<DrawThread<decltype(T::frameData)>> createDrawThread() override {
			return std::make_shared<T::DrawThread>();
		}
	};


	template<typename T> void XApp<T>::setup() {
		updateThread = createUpdateThread();
		drawThread = createDrawThread();

		onSetup();
		updateThread->onSetup();
		drawThread->onSetup();

		mainLoopThread = std::make_shared<std::thread>([&] {
			this->mainLoop();
			});
	}

	template<typename T> void XApp<T>::draw() {
		std::lock_guard lock(mtx);
		drawThread->onDraw(frameDataBuffer);
	}

	template<typename T> void XApp<T>::cleanup() {
		if (mainLoopThread) {
			threadClosing = true;
			mainLoopThread->join();
		}
		drawThread->onCleanup();
		updateThread->onCleanup();
		onCleanup();
	}

	template<typename T> void XApp<T>::mainLoop() {
		float elapsed = 0.0f;

		while (true) {
			updateThread->onUpdate(&frameData);

			{
				std::lock_guard lock(mtx);
				frameDataBuffer = frameData;
			}

			std::this_thread::yield();

			if (threadClosing) { break; }
		}
	}

}