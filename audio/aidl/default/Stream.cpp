/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AHAL_Stream"
#include <android-base/logging.h>
#include <utils/SystemClock.h>

#include "core-impl/Module.h"
#include "core-impl/Stream.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioOffloadInfo;

namespace aidl::android::hardware::audio::core {

void StreamContext::fillDescriptor(StreamDescriptor* desc) {
    if (mCommandMQ) {
        desc->command = mCommandMQ->dupeDesc();
    }
    if (mReplyMQ) {
        desc->reply = mReplyMQ->dupeDesc();
    }
    if (mDataMQ) {
        desc->frameSizeBytes = mFrameSize;
        desc->bufferSizeFrames =
                mDataMQ->getQuantumCount() * mDataMQ->getQuantumSize() / mFrameSize;
        desc->audio.set<StreamDescriptor::AudioBuffer::Tag::fmq>(mDataMQ->dupeDesc());
    }
}

bool StreamContext::isValid() const {
    if (mCommandMQ && !mCommandMQ->isValid()) {
        LOG(ERROR) << "command FMQ is invalid";
        return false;
    }
    if (mReplyMQ && !mReplyMQ->isValid()) {
        LOG(ERROR) << "reply FMQ is invalid";
        return false;
    }
    if (mFrameSize == 0) {
        LOG(ERROR) << "frame size is not set";
        return false;
    }
    if (mDataMQ && !mDataMQ->isValid()) {
        LOG(ERROR) << "data FMQ is invalid";
        return false;
    }
    return true;
}

void StreamContext::reset() {
    mCommandMQ.reset();
    mReplyMQ.reset();
    mDataMQ.reset();
}

std::string StreamWorkerCommonLogic::init() {
    if (mCommandMQ == nullptr) return "Command MQ is null";
    if (mReplyMQ == nullptr) return "Reply MQ is null";
    if (mDataMQ == nullptr) return "Data MQ is null";
    if (sizeof(decltype(mDataBuffer)::element_type) != mDataMQ->getQuantumSize()) {
        return "Unexpected Data MQ quantum size: " + std::to_string(mDataMQ->getQuantumSize());
    }
    mDataBufferSize = mDataMQ->getQuantumCount() * mDataMQ->getQuantumSize();
    mDataBuffer.reset(new (std::nothrow) int8_t[mDataBufferSize]);
    if (mDataBuffer == nullptr) {
        return "Failed to allocate data buffer for element count " +
               std::to_string(mDataMQ->getQuantumCount()) +
               ", size in bytes: " + std::to_string(mDataBufferSize);
    }
    return "";
}

void StreamWorkerCommonLogic::populateReply(StreamDescriptor::Reply* reply,
                                            bool isConnected) const {
    if (isConnected) {
        reply->status = STATUS_OK;
        reply->observable.frames = mFrameCount;
        reply->observable.timeNs = ::android::elapsedRealtimeNano();
    } else {
        reply->status = STATUS_NO_INIT;
    }
}

const std::string StreamInWorkerLogic::kThreadName = "reader";

StreamInWorkerLogic::Status StreamInWorkerLogic::cycle() {
    StreamDescriptor::Command command{};
    if (!mCommandMQ->readBlocking(&command, 1)) {
        LOG(ERROR) << __func__ << ": reading of command from MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    StreamDescriptor::Reply reply{};
    if (static_cast<int32_t>(command.code) == StreamContext::COMMAND_EXIT &&
        command.fmqByteCount == mInternalCommandCookie) {
        LOG(DEBUG) << __func__ << ": received EXIT command";
        setClosed();
        // This is an internal command, no need to reply.
        return Status::EXIT;
    } else if (command.code == StreamDescriptor::CommandCode::START && command.fmqByteCount >= 0) {
        LOG(DEBUG) << __func__ << ": received START read command";
        if (mState == StreamDescriptor::State::IDLE || mState == StreamDescriptor::State::STANDBY ||
            mState == StreamDescriptor::State::PAUSED) {
            populateReply(&reply, mIsConnected);
            mState = StreamDescriptor::State::READY;
        } else {
            LOG(WARNING) << __func__ << ": START command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else if (command.code == StreamDescriptor::CommandCode::BURST && command.fmqByteCount >= 0) {
        LOG(DEBUG) << __func__ << ": received BURST read command for " << command.fmqByteCount
                   << " bytes";
        if (mState == StreamDescriptor::State::READY) {
            usleep(3000);  // Simulate a blocking call into the driver.
            // Can switch the state to ERROR if a driver error occurs.
            const size_t byteCount = std::min({static_cast<size_t>(command.fmqByteCount),
                                               mDataMQ->availableToWrite(), mDataBufferSize});
            const bool isConnected = mIsConnected;
            // Simulate reading of data, or provide zeroes if the stream is not connected.
            for (size_t i = 0; i < byteCount; ++i) {
                using buffer_type = decltype(mDataBuffer)::element_type;
                constexpr int kBufferValueRange = std::numeric_limits<buffer_type>::max() -
                                                  std::numeric_limits<buffer_type>::min() + 1;
                mDataBuffer[i] = isConnected ? (std::rand() % kBufferValueRange) +
                                                       std::numeric_limits<buffer_type>::min()
                                             : 0;
            }
            bool success = byteCount > 0 ? mDataMQ->write(&mDataBuffer[0], byteCount) : true;
            if (success) {
                LOG(DEBUG) << __func__ << ": writing of " << byteCount << " bytes into data MQ"
                           << " succeeded; connected? " << isConnected;
                // Frames are provided and counted regardless of connection status.
                reply.fmqByteCount = byteCount;
                mFrameCount += byteCount / mFrameSize;
                populateReply(&reply, isConnected);
            } else {
                LOG(WARNING) << __func__ << ": writing of " << byteCount
                             << " bytes of data to MQ failed";
                reply.status = STATUS_NOT_ENOUGH_DATA;
            }
            reply.latencyMs = Module::kLatencyMs;
        } else {
            LOG(WARNING) << __func__ << ": BURST command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else if (command.code == StreamDescriptor::CommandCode::DRAIN && command.fmqByteCount == 0) {
        LOG(DEBUG) << __func__ << ": received DRAIN read command";
        if (mState == StreamDescriptor::State::READY || mState == StreamDescriptor::State::PAUSED) {
            populateReply(&reply, mIsConnected);
            usleep(3000);  // Simulate a blocking call into the driver.
            // Can switch the state to ERROR if a driver error occurs.
            mState = StreamDescriptor::State::IDLE;
        } else {
            LOG(WARNING) << __func__ << ": DRAIN command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else if (command.code == StreamDescriptor::CommandCode::STANDBY &&
               command.fmqByteCount == 0) {
        LOG(DEBUG) << __func__ << ": received STANDBY read command";
        if (mState == StreamDescriptor::State::IDLE) {
            populateReply(&reply, mIsConnected);
            usleep(3000);  // Simulate a blocking call into the driver.
            mState = StreamDescriptor::State::STANDBY;
        } else {
            LOG(WARNING) << __func__ << ": STANDBY command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else if (command.code == StreamDescriptor::CommandCode::PAUSE && command.fmqByteCount == 0) {
        LOG(DEBUG) << __func__ << ": received PAUSE read command";
        if (mState == StreamDescriptor::State::READY) {
            populateReply(&reply, mIsConnected);
            mState = StreamDescriptor::State::PAUSED;
        } else {
            LOG(WARNING) << __func__ << ": PAUSE command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else if (command.code == StreamDescriptor::CommandCode::FLUSH && command.fmqByteCount == 0) {
        LOG(DEBUG) << __func__ << ": received FLUSH read command";
        if (mState == StreamDescriptor::State::IDLE || mState == StreamDescriptor::State::STANDBY ||
            mState == StreamDescriptor::State::PAUSED) {
            populateReply(&reply, mIsConnected);
            if (mState == StreamDescriptor::State::PAUSED) {
                mState = StreamDescriptor::State::IDLE;
            }
        } else {
            LOG(WARNING) << __func__ << ": FLUSH command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else {
        LOG(WARNING) << __func__ << ": invalid command (" << command.toString()
                     << ") or count: " << command.fmqByteCount;
        reply.status = STATUS_BAD_VALUE;
    }
    reply.state = mState;
    LOG(DEBUG) << __func__ << ": writing reply " << reply.toString();
    if (!mReplyMQ->writeBlocking(&reply, 1)) {
        LOG(ERROR) << __func__ << ": writing of reply " << reply.toString() << " to MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    return Status::CONTINUE;
}

const std::string StreamOutWorkerLogic::kThreadName = "writer";

StreamOutWorkerLogic::Status StreamOutWorkerLogic::cycle() {
    StreamDescriptor::Command command{};
    if (!mCommandMQ->readBlocking(&command, 1)) {
        LOG(ERROR) << __func__ << ": reading of command from MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    StreamDescriptor::Reply reply{};
    if (static_cast<int32_t>(command.code) == StreamContext::COMMAND_EXIT &&
        command.fmqByteCount == mInternalCommandCookie) {
        LOG(DEBUG) << __func__ << ": received EXIT command";
        setClosed();
        // This is an internal command, no need to reply.
        return Status::EXIT;
    } else if (command.code == StreamDescriptor::CommandCode::START && command.fmqByteCount >= 0) {
        LOG(DEBUG) << __func__ << ": received START read command";
        if (mState == StreamDescriptor::State::IDLE || mState == StreamDescriptor::State::STANDBY ||
            mState == StreamDescriptor::State::PAUSED) {
            populateReply(&reply, mIsConnected);
            mState = StreamDescriptor::State::READY;
        } else {
            LOG(WARNING) << __func__ << ": START command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else if (command.code == StreamDescriptor::CommandCode::BURST && command.fmqByteCount >= 0) {
        LOG(DEBUG) << __func__ << ": received BURST write command for " << command.fmqByteCount
                   << " bytes";
        if (mState == StreamDescriptor::State::READY) {
            if (!write(std::min({static_cast<size_t>(command.fmqByteCount),
                                 mDataMQ->availableToRead(), mDataBufferSize}),
                       &reply)) {
                mState = StreamDescriptor::State::ERROR;
            }
        } else {
            LOG(WARNING) << __func__ << ": BURST command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else if (command.code == StreamDescriptor::CommandCode::DRAIN && command.fmqByteCount == 0) {
        LOG(DEBUG) << __func__ << ": received DRAIN write command";
        if (mState == StreamDescriptor::State::READY || mState == StreamDescriptor::State::PAUSED) {
            bool fatal = false;
            while (mDataMQ->availableToRead() > 0) {
                if (!write(std::min({mDataMQ->availableToRead(), mDataBufferSize}), &reply)) {
                    fatal = true;
                }
            }
            if (!fatal) {
                populateReply(&reply, mIsConnected);
                mState = StreamDescriptor::State::IDLE;
            } else {
                mState = StreamDescriptor::State::ERROR;
            }
        } else {
            LOG(WARNING) << __func__ << ": DRAIN command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else if (command.code == StreamDescriptor::CommandCode::STANDBY &&
               command.fmqByteCount == 0) {
        LOG(DEBUG) << __func__ << ": received STANDBY write command";
        if (mState == StreamDescriptor::State::IDLE) {
            populateReply(&reply, mIsConnected);
            usleep(1000);  // Simulate a blocking call into the driver.
            mState = StreamDescriptor::State::STANDBY;
        } else {
            LOG(WARNING) << __func__ << ": STANDBY command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else if (command.code == StreamDescriptor::CommandCode::PAUSE && command.fmqByteCount == 0) {
        LOG(DEBUG) << __func__ << ": received PAUSE write command";
        if (mState == StreamDescriptor::State::READY) {
            populateReply(&reply, mIsConnected);
            mState = StreamDescriptor::State::PAUSED;
        } else {
            LOG(WARNING) << __func__ << ": PAUSE command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else if (command.code == StreamDescriptor::CommandCode::FLUSH && command.fmqByteCount == 0) {
        LOG(DEBUG) << __func__ << ": received FLUSH write command";
        if (mState == StreamDescriptor::State::IDLE || mState == StreamDescriptor::State::STANDBY ||
            mState == StreamDescriptor::State::PAUSED) {
            if (mDataMQ->availableToRead() > 0) {
                std::vector<int8_t> temp(mDataMQ->availableToRead());
                (void)mDataMQ->read(&temp[0], temp.size());
                reply.fmqByteCount += temp.size();
                mFrameCount += temp.size() / mFrameSize;
            }
            populateReply(&reply, mIsConnected);
            if (mState == StreamDescriptor::State::PAUSED) {
                mState = StreamDescriptor::State::IDLE;
            }
        } else {
            LOG(WARNING) << __func__ << ": FLUSH command can not be handled in the state "
                         << toString(mState);
            reply.status = STATUS_INVALID_OPERATION;
        }
    } else {
        LOG(WARNING) << __func__ << ": invalid command (" << command.toString()
                     << ") or count: " << command.fmqByteCount;
        reply.status = STATUS_BAD_VALUE;
    }
    reply.state = mState;
    LOG(DEBUG) << __func__ << ": writing reply " << reply.toString();
    if (!mReplyMQ->writeBlocking(&reply, 1)) {
        LOG(ERROR) << __func__ << ": writing of reply " << reply.toString() << " to MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    return Status::CONTINUE;
}

bool StreamOutWorkerLogic::write(size_t byteCount, StreamDescriptor::Reply* reply) {
    bool fatal = false;
    bool success = byteCount > 0 ? mDataMQ->read(&mDataBuffer[0], byteCount) : true;
    if (success) {
        const bool isConnected = mIsConnected;
        LOG(DEBUG) << __func__ << ": reading of " << byteCount << " bytes from data MQ"
                   << " succeeded; connected? " << isConnected;
        // Frames are consumed and counted regardless of connection status.
        reply->fmqByteCount += byteCount;
        mFrameCount += byteCount / mFrameSize;
        populateReply(reply, isConnected);
        usleep(3000);  // Simulate a blocking call into the driver.
        // Set 'fatal = true' if a driver error occurs.
    } else {
        LOG(WARNING) << __func__ << ": reading of " << byteCount << " bytes of data from MQ failed";
        reply->status = STATUS_NOT_ENOUGH_DATA;
    }
    reply->latencyMs = Module::kLatencyMs;
    return !fatal;
}

template <class Metadata, class StreamWorker>
StreamCommon<Metadata, StreamWorker>::~StreamCommon() {
    if (!isClosed()) {
        LOG(ERROR) << __func__ << ": stream was not closed prior to destruction, resource leak";
        stopWorker();
        // The worker and the context should clean up by themselves via destructors.
    }
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommon<Metadata, StreamWorker>::close() {
    LOG(DEBUG) << __func__;
    if (!isClosed()) {
        stopWorker();
        LOG(DEBUG) << __func__ << ": joining the worker thread...";
        mWorker.stop();
        LOG(DEBUG) << __func__ << ": worker thread joined";
        mContext.reset();
        mWorker.setClosed();
        return ndk::ScopedAStatus::ok();
    } else {
        LOG(ERROR) << __func__ << ": stream was already closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
}

template <class Metadata, class StreamWorker>
void StreamCommon<Metadata, StreamWorker>::stopWorker() {
    if (auto commandMQ = mContext.getCommandMQ(); commandMQ != nullptr) {
        LOG(DEBUG) << __func__ << ": asking the worker to exit...";
        StreamDescriptor::Command cmd;
        cmd.code = StreamDescriptor::CommandCode(StreamContext::COMMAND_EXIT);
        cmd.fmqByteCount = mContext.getInternalCommandCookie();
        // Note: never call 'pause' and 'resume' methods of StreamWorker
        // in the HAL implementation. These methods are to be used by
        // the client side only. Preventing the worker loop from running
        // on the HAL side can cause a deadlock.
        if (!commandMQ->writeBlocking(&cmd, 1)) {
            LOG(ERROR) << __func__ << ": failed to write exit command to the MQ";
        }
        LOG(DEBUG) << __func__ << ": done";
    }
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommon<Metadata, StreamWorker>::updateMetadata(const Metadata& metadata) {
    LOG(DEBUG) << __func__;
    if (!isClosed()) {
        mMetadata = metadata;
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << __func__ << ": stream was closed";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}

StreamIn::StreamIn(const SinkMetadata& sinkMetadata, StreamContext context)
    : StreamCommon<SinkMetadata, StreamInWorker>(sinkMetadata, std::move(context)) {
    LOG(DEBUG) << __func__;
}

StreamOut::StreamOut(const SourceMetadata& sourceMetadata, StreamContext context,
                     const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamCommon<SourceMetadata, StreamOutWorker>(sourceMetadata, std::move(context)),
      mOffloadInfo(offloadInfo) {
    LOG(DEBUG) << __func__;
}

}  // namespace aidl::android::hardware::audio::core
