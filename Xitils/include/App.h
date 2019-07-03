#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include <thread>

#define XITILS_APP(x) CINDER_APP(x, RendererGl)

namespace Xitils::App {

	template<typename T> class XApp : public ci::app::App {
	public:
		virtual void OnSetup(T* frameParams) {}
		virtual void OnCleanUp() {}
		virtual void OnUpdate(T* frameParams) {} // メインスレッド以外から呼ばれるので注意
		virtual void OnDraw(const T& frameParams) {} // こちらはメインスレッドから呼ばれる

		void setup() override;
		void draw() override;
		void cleanup() override;
		void update() override;

		void mainLoop();

	protected:
	private:
		// 更新用スレッドと描画用スレッドが情報をやり取りする際、
		// frameParams 内のメンバ変数としてデータを渡す必要がある。
		// frameParams はバッファされてスレッドセーフに処理される。
		// これ以外の情報を OnUpdate と OnDraw でまたいで使ってはいけない。
		T frameParams;

		T frameParamsBuffer;
		std::mutex mtx;

		ci::Surface surfaceBuffer;
		ci::gl::TextureRef texture;
		float elapsedBuffer = 0.0f;

		std::shared_ptr<std::thread> mainLoopThread;
		bool threadClosing = false;

	};

	template<typename T> void XApp<T>::setup() {
		OnSetup(&frameParams);

		mainLoopThread = std::make_shared<std::thread>([&] {
			this->mainLoop();
			});
	}

	template<typename T> void XApp<T>::draw() {
		std::lock_guard lock(mtx);
		OnDraw(frameParamsBuffer);
	}

	template<typename T> void XApp<T>::cleanup() {
		if (mainLoopThread) {
			threadClosing = true;
			mainLoopThread->join();
		}
	}

	template<typename T> void XApp<T>::update() {
	}

	template<typename T> void XApp<T>::mainLoop() {
		float elapsed = 0.0f;

		while (true) {
			OnUpdate(&frameParams);

			{
				std::lock_guard lock(mtx);
				frameParamsBuffer = frameParams;
			}

			std::this_thread::yield();

			if (threadClosing) { break; }
		}
	}

}