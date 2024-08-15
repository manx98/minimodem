/*
 * simpleaudio-sndio.c
 *
 * Copyright (C) 2011-2020 Kamal Mostafa <kamal@whence.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <io.h>
#include <windows.h>
#if WIN32

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>

#include "simpleaudio.h"
#include "simpleaudio_internal.h"
#include <mutex>


/*
 * sndio backend for simpleaudio
 */

struct SimpleaudioWin
{
	WAVEFORMATEX waveFormat;
	HWAVEOUT hWaveOut{nullptr};
	HWAVEIN hWaveIn{nullptr};
	WAVEHDR waveHeader;
	HANDLE wait{nullptr};
	std::mutex lock;
	int fd;
	bool need_wait{false};
};

static ssize_t
sa_win_read(simpleaudio* sa, void* buf, size_t nframes)
{
	size_t nbytes = nframes * sa->backend_framesize;
	return nframes;
}


static ssize_t
sa_win_write(simpleaudio* sa, void* buf, size_t nframes)
{
	auto handle = static_cast<SimpleaudioWin*>(sa->backend_handle);
	std::lock_guard<std::mutex> _(handle->lock);
	if(handle->need_wait)
	{
		// 阻塞直到音频播放完成
		auto ret = waveOutReset(handle->hWaveOut);
		if(ret != MMSYSERR_NOERROR){
			fprintf(stderr, "Failed to reset audio device, MMRESULT=%d\n", ret);
			return -1;
		}
	} else
	{
		handle->need_wait = true;
	}
	handle->waveHeader.lpData = static_cast<LPSTR>(buf);
	handle->waveHeader.dwBufferLength = nframes * sa->backend_framesize;
	handle->waveHeader.dwFlags = 0;
	handle->waveHeader.dwLoops = 0;
	auto ret = waveOutPrepareHeader(handle->hWaveOut, &handle->waveHeader, sizeof(WAVEHDR));
	if (ret != MMSYSERR_NOERROR) {
		fprintf(stderr, "Failed to prepare audio data, MMRESULT=%d\n", ret);
		return -1;
	}

	ret = waveOutWrite(handle->hWaveOut, &handle->waveHeader, sizeof(WAVEHDR));
	if (ret != MMSYSERR_NOERROR) {
		fprintf(stderr, "Failed to write audio data, MMRESULT=%d\n", ret);
		return -1;
	}
	return nframes;
}


static void
sa_win_close(simpleaudio* sa)
{
	auto* handle = static_cast<SimpleaudioWin*>(sa->backend_handle);
	// 清理资源并关闭音频设备
	if (handle->hWaveOut)
	{
		waveOutUnprepareHeader(handle->hWaveOut, &handle->waveHeader, sizeof(WAVEHDR));
		waveOutClose(handle->hWaveOut);
	}
	if (handle->hWaveIn)
	{
		waveInUnprepareHeader(handle->hWaveIn, &handle->waveHeader, sizeof(WAVEHDR));
		waveInClose(handle->hWaveIn);
	}
	if(handle->wait)
	{
		CloseHandle(handle->wait);
	}
	delete handle;
	sa->backend_handle = nullptr;
}


static int
sa_win_open_stream(
	simpleaudio* sa,
	const char* backend_device,
	sa_direction_t sa_stream_direction,
	sa_format_t sa_format,
	unsigned int rate, unsigned int channels,
	char* app_name, char* stream_name)
{
	auto handle = new SimpleaudioWin();
	handle->waveHeader.dwFlags = WHDR_DONE;
	sa->backend_handle = handle;
	// 设置 PCM 格式
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = channels; // 单声道
	waveFormat.nSamplesPerSec = rate; // 采样率
	switch ( sa->format ) {
	case SA_SAMPLE_FORMAT_FLOAT:
		waveFormat.wBitsPerSample = 32; // 每个样本32位
		break;
	case SA_SAMPLE_FORMAT_S16:
		waveFormat.wBitsPerSample = 16; // 每个样本16位
		break;
	default:
		assert(0);
	}
	handle->wait = CreateEvent(nullptr, 0, 0, nullptr);
	waveFormat.nBlockAlign = waveFormat.nChannels * (waveFormat.wBitsPerSample / 8);
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;
	auto success = true;
	if (sa_stream_direction == SA_STREAM_RECORD)
	{
		if (waveInOpen(&handle->hWaveIn, WAVE_MAPPER, &waveFormat, (DWORD_PTR)handle->wait, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR)
		{
			fprintf(stderr, "Failed to open waveform audio device.\n");
			success = false;
		}
	}
	else
	{
		// 打开音频设备
		if (waveOutOpen(&handle->hWaveOut, WAVE_MAPPER, &waveFormat, (DWORD_PTR)handle->wait, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR)
		{
			fprintf(stderr, "Failed to open waveform audio device.\n");
			success = false;
		}
	}
	if (!success)
	{
		sa_win_close(sa);
	}
	else
	{
		sa->backend_framesize = sa->channels * sa->samplesize;
	}
	return 1;
}


const struct simpleaudio_backend simpleaudio_backend_win = {
	sa_win_open_stream,
	sa_win_read,
	sa_win_write,
	sa_win_close,
};

#endif /* USE_SNDIO */
