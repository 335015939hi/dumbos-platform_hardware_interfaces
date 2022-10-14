// FIXME: license file, or use the -l option to generate the files with the header.

// FIXME Remove this file if you don't need to translate types in this backend.

#pragma once

#include "android/hardware/media/c2/1.0/IComponentListener.h"
#include "android/hardware/media/c2/1.0/IComponentStore.h"
#include "android/hardware/media/c2/1.0/types.h"
#include "android/hardware/media/c2/1.2/types.h"
#include "android/hardware/media/c2/BaseBlock.h"
#include "android/hardware/media/c2/Block.h"
#include "android/hardware/media/c2/Buffer.h"
#include "android/hardware/media/c2/FieldDescriptor.h"
#include "android/hardware/media/c2/FieldId.h"
#include "android/hardware/media/c2/FieldSupportedValues.h"
#include "android/hardware/media/c2/FieldSupportedValuesQuery.h"
#include "android/hardware/media/c2/FieldSupportedValuesQueryResult.h"
#include "android/hardware/media/c2/FrameData.h"
#include "android/hardware/media/c2/IComponentListener.h"
#include "android/hardware/media/c2/IComponentStore.h"
#include "android/hardware/media/c2/InfoBuffer.h"
#include "android/hardware/media/c2/ParamDescriptor.h"
#include "android/hardware/media/c2/ParamField.h"
#include "android/hardware/media/c2/ParamFieldValues.h"
#include "android/hardware/media/c2/SettingResult.h"
#include "android/hardware/media/c2/Status.h"
#include "android/hardware/media/c2/StructDescriptor.h"
#include "android/hardware/media/c2/SurfaceSyncObj.h"
#include "android/hardware/media/c2/ValueRange.h"
#include "android/hardware/media/c2/Work.h"
#include "android/hardware/media/c2/WorkBundle.h"
#include "android/hardware/media/c2/WorkOrdinal.h"
#include "android/hardware/media/c2/Worklet.h"
#include <limits>

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldId& in, android::hardware::media::c2::FieldId* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::ParamField& in, android::hardware::media::c2::ParamField* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::ParamDescriptor& in, android::hardware::media::c2::ParamDescriptor* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::ValueRange& in, android::hardware::media::c2::ValueRange* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldSupportedValues& in, android::hardware::media::c2::FieldSupportedValues* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::ParamFieldValues& in, android::hardware::media::c2::ParamFieldValues* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldDescriptor& in, android::hardware::media::c2::FieldDescriptor* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldDescriptor::NamedValue& in, android::hardware::media::c2::FieldDescriptor::NamedValue* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::StructDescriptor& in, android::hardware::media::c2::StructDescriptor* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::SettingResult& in, android::hardware::media::c2::SettingResult* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::WorkOrdinal& in, android::hardware::media::c2::WorkOrdinal* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::BaseBlock& in, android::hardware::media::c2::BaseBlock* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::Block& in, android::hardware::media::c2::Block* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::Buffer& in, android::hardware::media::c2::Buffer* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::InfoBuffer& in, android::hardware::media::c2::InfoBuffer* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FrameData& in, android::hardware::media::c2::FrameData* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::Worklet& in, android::hardware::media::c2::Worklet* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::Work& in, android::hardware::media::c2::Work* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::WorkBundle& in, android::hardware::media::c2::WorkBundle* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldSupportedValuesQuery& in, android::hardware::media::c2::FieldSupportedValuesQuery* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldSupportedValuesQueryResult& in, android::hardware::media::c2::FieldSupportedValuesQueryResult* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_2::SurfaceSyncObj& in, android::hardware::media::c2::SurfaceSyncObj* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::IComponentListener::RenderedFrame& in, android::hardware::media::c2::IComponentListener::RenderedFrame* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::IComponentListener::InputBuffer& in, android::hardware::media::c2::IComponentListener::InputBuffer* out);
__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::IComponentStore::ComponentTraits& in, android::hardware::media::c2::IComponentStore::ComponentTraits* out);

}  // namespace android::h2a
