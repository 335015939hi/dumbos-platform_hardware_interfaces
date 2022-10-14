// FIXME: license file, or use the -l option to generate the files with the header.

// FIXME Remove this file if you don't need to translate types in this backend.

#include "android/hardware/media/c2/translate-ndk.h"

namespace android::h2a {

static_assert(aidl::android::hardware::media::c2::Status::OK == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::OK));
static_assert(aidl::android::hardware::media::c2::Status::BAD_VALUE == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::BAD_VALUE));
static_assert(aidl::android::hardware::media::c2::Status::BAD_INDEX == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::BAD_INDEX));
static_assert(aidl::android::hardware::media::c2::Status::CANNOT_DO == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::CANNOT_DO));
static_assert(aidl::android::hardware::media::c2::Status::DUPLICATE == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::DUPLICATE));
static_assert(aidl::android::hardware::media::c2::Status::NOT_FOUND == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::NOT_FOUND));
static_assert(aidl::android::hardware::media::c2::Status::BAD_STATE == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::BAD_STATE));
static_assert(aidl::android::hardware::media::c2::Status::BLOCKING == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::BLOCKING));
static_assert(aidl::android::hardware::media::c2::Status::NO_MEMORY == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::NO_MEMORY));
static_assert(aidl::android::hardware::media::c2::Status::REFUSED == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::REFUSED));
static_assert(aidl::android::hardware::media::c2::Status::TIMED_OUT == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::TIMED_OUT));
static_assert(aidl::android::hardware::media::c2::Status::OMITTED == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::OMITTED));
static_assert(aidl::android::hardware::media::c2::Status::CORRUPTED == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::CORRUPTED));
static_assert(aidl::android::hardware::media::c2::Status::NO_INIT == static_cast<aidl::android::hardware::media::c2::Status>(::android::hardware::media::c2::V1_0::Status::NO_INIT));

static_assert(aidl::android::hardware::media::c2::ParamDescriptor::Attrib::REQUIRED == static_cast<aidl::android::hardware::media::c2::ParamDescriptor::Attrib>(::android::hardware::media::c2::V1_0::ParamDescriptor::Attrib::REQUIRED));
static_assert(aidl::android::hardware::media::c2::ParamDescriptor::Attrib::PERSISTENT == static_cast<aidl::android::hardware::media::c2::ParamDescriptor::Attrib>(::android::hardware::media::c2::V1_0::ParamDescriptor::Attrib::PERSISTENT));
static_assert(aidl::android::hardware::media::c2::ParamDescriptor::Attrib::STRICT == static_cast<aidl::android::hardware::media::c2::ParamDescriptor::Attrib>(::android::hardware::media::c2::V1_0::ParamDescriptor::Attrib::STRICT));
static_assert(aidl::android::hardware::media::c2::ParamDescriptor::Attrib::READ_ONLY == static_cast<aidl::android::hardware::media::c2::ParamDescriptor::Attrib>(::android::hardware::media::c2::V1_0::ParamDescriptor::Attrib::READ_ONLY));
static_assert(aidl::android::hardware::media::c2::ParamDescriptor::Attrib::HIDDEN == static_cast<aidl::android::hardware::media::c2::ParamDescriptor::Attrib>(::android::hardware::media::c2::V1_0::ParamDescriptor::Attrib::HIDDEN));
static_assert(aidl::android::hardware::media::c2::ParamDescriptor::Attrib::INTERNAL == static_cast<aidl::android::hardware::media::c2::ParamDescriptor::Attrib>(::android::hardware::media::c2::V1_0::ParamDescriptor::Attrib::INTERNAL));
static_assert(aidl::android::hardware::media::c2::ParamDescriptor::Attrib::CONST == static_cast<aidl::android::hardware::media::c2::ParamDescriptor::Attrib>(::android::hardware::media::c2::V1_0::ParamDescriptor::Attrib::CONST));

static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::NO_INIT == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::NO_INIT));
static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::INT32 == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::INT32));
static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::UINT32 == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::UINT32));
static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::CNTR32 == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::CNTR32));
static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::INT64 == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::INT64));
static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::UINT64 == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::UINT64));
static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::CNTR64 == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::CNTR64));
static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::FLOAT == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::FLOAT));
static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::STRING == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::STRING));
static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::BLOB == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::BLOB));
static_assert(aidl::android::hardware::media::c2::FieldDescriptor::Type::STRUCT == static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(::android::hardware::media::c2::V1_0::FieldDescriptor::Type::STRUCT));

static_assert(aidl::android::hardware::media::c2::SettingResult::Failure::BAD_TYPE == static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(::android::hardware::media::c2::V1_0::SettingResult::Failure::BAD_TYPE));
static_assert(aidl::android::hardware::media::c2::SettingResult::Failure::BAD_PORT == static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(::android::hardware::media::c2::V1_0::SettingResult::Failure::BAD_PORT));
static_assert(aidl::android::hardware::media::c2::SettingResult::Failure::BAD_INDEX == static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(::android::hardware::media::c2::V1_0::SettingResult::Failure::BAD_INDEX));
static_assert(aidl::android::hardware::media::c2::SettingResult::Failure::READ_ONLY == static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(::android::hardware::media::c2::V1_0::SettingResult::Failure::READ_ONLY));
static_assert(aidl::android::hardware::media::c2::SettingResult::Failure::MISMATCH == static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(::android::hardware::media::c2::V1_0::SettingResult::Failure::MISMATCH));
static_assert(aidl::android::hardware::media::c2::SettingResult::Failure::BAD_VALUE == static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(::android::hardware::media::c2::V1_0::SettingResult::Failure::BAD_VALUE));
static_assert(aidl::android::hardware::media::c2::SettingResult::Failure::CONFLICT == static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(::android::hardware::media::c2::V1_0::SettingResult::Failure::CONFLICT));
static_assert(aidl::android::hardware::media::c2::SettingResult::Failure::UNSUPPORTED == static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(::android::hardware::media::c2::V1_0::SettingResult::Failure::UNSUPPORTED));
static_assert(aidl::android::hardware::media::c2::SettingResult::Failure::INFO_BAD_VALUE == static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(::android::hardware::media::c2::V1_0::SettingResult::Failure::INFO_BAD_VALUE));
static_assert(aidl::android::hardware::media::c2::SettingResult::Failure::INFO_CONFLICT == static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(::android::hardware::media::c2::V1_0::SettingResult::Failure::INFO_CONFLICT));

static_assert(aidl::android::hardware::media::c2::FrameData::Flags::DROP_FRAME == static_cast<aidl::android::hardware::media::c2::FrameData::Flags>(::android::hardware::media::c2::V1_0::FrameData::Flags::DROP_FRAME));
static_assert(aidl::android::hardware::media::c2::FrameData::Flags::END_OF_STREAM == static_cast<aidl::android::hardware::media::c2::FrameData::Flags>(::android::hardware::media::c2::V1_0::FrameData::Flags::END_OF_STREAM));
static_assert(aidl::android::hardware::media::c2::FrameData::Flags::DISCARD_FRAME == static_cast<aidl::android::hardware::media::c2::FrameData::Flags>(::android::hardware::media::c2::V1_0::FrameData::Flags::DISCARD_FRAME));
static_assert(aidl::android::hardware::media::c2::FrameData::Flags::FLAG_INCOMPLETE == static_cast<aidl::android::hardware::media::c2::FrameData::Flags>(::android::hardware::media::c2::V1_0::FrameData::Flags::FLAG_INCOMPLETE));
static_assert(aidl::android::hardware::media::c2::FrameData::Flags::CODEC_CONFIG == static_cast<aidl::android::hardware::media::c2::FrameData::Flags>(::android::hardware::media::c2::V1_0::FrameData::Flags::CODEC_CONFIG));

static_assert(aidl::android::hardware::media::c2::FieldSupportedValuesQuery::Type::POSSIBLE == static_cast<aidl::android::hardware::media::c2::FieldSupportedValuesQuery::Type>(::android::hardware::media::c2::V1_0::FieldSupportedValuesQuery::Type::POSSIBLE));
static_assert(aidl::android::hardware::media::c2::FieldSupportedValuesQuery::Type::CURRENT == static_cast<aidl::android::hardware::media::c2::FieldSupportedValuesQuery::Type>(::android::hardware::media::c2::V1_0::FieldSupportedValuesQuery::Type::CURRENT));

static_assert(aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Kind::OTHER == static_cast<aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Kind>(::android::hardware::media::c2::V1_0::IComponentStore::ComponentTraits::Kind::OTHER));
static_assert(aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Kind::DECODER == static_cast<aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Kind>(::android::hardware::media::c2::V1_0::IComponentStore::ComponentTraits::Kind::DECODER));
static_assert(aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Kind::ENCODER == static_cast<aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Kind>(::android::hardware::media::c2::V1_0::IComponentStore::ComponentTraits::Kind::ENCODER));

static_assert(aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Domain::OTHER == static_cast<aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Domain>(::android::hardware::media::c2::V1_0::IComponentStore::ComponentTraits::Domain::OTHER));
static_assert(aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Domain::VIDEO == static_cast<aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Domain>(::android::hardware::media::c2::V1_0::IComponentStore::ComponentTraits::Domain::VIDEO));
static_assert(aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Domain::AUDIO == static_cast<aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Domain>(::android::hardware::media::c2::V1_0::IComponentStore::ComponentTraits::Domain::AUDIO));
static_assert(aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Domain::IMAGE == static_cast<aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Domain>(::android::hardware::media::c2::V1_0::IComponentStore::ComponentTraits::Domain::IMAGE));

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldId& in, aidl::android::hardware::media::c2::FieldId* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.offset > std::numeric_limits<int32_t>::max() || in.offset < 0) {
        return false;
    }
    out->offset = static_cast<int32_t>(in.offset);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.size > std::numeric_limits<int32_t>::max() || in.size < 0) {
        return false;
    }
    out->size = static_cast<int32_t>(in.size);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::ParamField& in, aidl::android::hardware::media::c2::ParamField* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.index > std::numeric_limits<int32_t>::max() || in.index < 0) {
        return false;
    }
    out->index = static_cast<int32_t>(in.index);
    if (!translate(in.fieldId, &out->fieldId)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::ParamDescriptor& in, aidl::android::hardware::media::c2::ParamDescriptor* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.index > std::numeric_limits<int32_t>::max() || in.index < 0) {
        return false;
    }
    out->index = static_cast<int32_t>(in.index);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.attrib > std::numeric_limits<int32_t>::max() || in.attrib < 0) {
        return false;
    }
    out->attrib = static_cast<aidl::android::hardware::media::c2::ParamDescriptor::Attrib>(in.attrib);
    out->name = in.name;
    {
        size_t size = in.dependencies.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.dependencies[i] > std::numeric_limits<int32_t>::max() || in.dependencies[i] < 0) {
                return false;
            }
            out->dependencies.push_back(static_cast<int32_t>(in.dependencies[i]));
        }
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::ValueRange& in, aidl::android::hardware::media::c2::ValueRange* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.min > std::numeric_limits<int64_t>::max() || in.min < 0) {
        return false;
    }
    out->min = static_cast<int64_t>(in.min);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.max > std::numeric_limits<int64_t>::max() || in.max < 0) {
        return false;
    }
    out->max = static_cast<int64_t>(in.max);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.step > std::numeric_limits<int64_t>::max() || in.step < 0) {
        return false;
    }
    out->step = static_cast<int64_t>(in.step);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.num > std::numeric_limits<int64_t>::max() || in.num < 0) {
        return false;
    }
    out->num = static_cast<int64_t>(in.num);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.denom > std::numeric_limits<int64_t>::max() || in.denom < 0) {
        return false;
    }
    out->denom = static_cast<int64_t>(in.denom);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldSupportedValues& in, aidl::android::hardware::media::c2::FieldSupportedValues* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::media::c2::V1_0::FieldSupportedValues::hidl_discriminator::empty:
            // Nothing to translate for Monostate.
            break;
        case ::android::hardware::media::c2::V1_0::FieldSupportedValues::hidl_discriminator::range:
            {
            aidl::android::hardware::media::c2::ValueRange range;
            if (!translate(in.range(), &range)) return false;
            out->set<aidl::android::hardware::media::c2::FieldSupportedValues::range>(range);
            }
            break;
        case ::android::hardware::media::c2::V1_0::FieldSupportedValues::hidl_discriminator::values:
            {
                out->set<aidl::android::hardware::media::c2::FieldSupportedValues::values>();
                size_t size = in.values().size();
                for (size_t i = 0; i < size; i++) {
                    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
                    if (in.values()[i] > std::numeric_limits<int64_t>::max() || in.values()[i] < 0) {
                        return false;
                    }
                    out->get<aidl::android::hardware::media::c2::FieldSupportedValues::values>().push_back(static_cast<int64_t>(in.values()[i]));
                }
            }
            break;
        case ::android::hardware::media::c2::V1_0::FieldSupportedValues::hidl_discriminator::flags:
            {
                out->set<aidl::android::hardware::media::c2::FieldSupportedValues::flags>();
                size_t size = in.flags().size();
                for (size_t i = 0; i < size; i++) {
                    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
                    if (in.flags()[i] > std::numeric_limits<int64_t>::max() || in.flags()[i] < 0) {
                        return false;
                    }
                    out->get<aidl::android::hardware::media::c2::FieldSupportedValues::flags>().push_back(static_cast<int64_t>(in.flags()[i]));
                }
            }
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::ParamFieldValues& in, aidl::android::hardware::media::c2::ParamFieldValues* out) {
    if (!translate(in.paramOrField, &out->paramOrField)) return false;
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: values
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldDescriptor& in, aidl::android::hardware::media::c2::FieldDescriptor* out) {
    if (!translate(in.fieldId, &out->fieldId)) return false;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.type > std::numeric_limits<int32_t>::max() || in.type < 0) {
        return false;
    }
    out->type = static_cast<aidl::android::hardware::media::c2::FieldDescriptor::Type>(in.type);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.structIndex > std::numeric_limits<int32_t>::max() || in.structIndex < 0) {
        return false;
    }
    out->structIndex = static_cast<int32_t>(in.structIndex);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.extent > std::numeric_limits<int32_t>::max() || in.extent < 0) {
        return false;
    }
    out->extent = static_cast<int32_t>(in.extent);
    out->name = in.name;
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: namedValues
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldDescriptor::NamedValue& in, aidl::android::hardware::media::c2::FieldDescriptor::NamedValue* out) {
    out->name = in.name;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.value > std::numeric_limits<int64_t>::max() || in.value < 0) {
        return false;
    }
    out->value = static_cast<int64_t>(in.value);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::StructDescriptor& in, aidl::android::hardware::media::c2::StructDescriptor* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.type > std::numeric_limits<int32_t>::max() || in.type < 0) {
        return false;
    }
    out->type = static_cast<int32_t>(in.type);
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: fields
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::SettingResult& in, aidl::android::hardware::media::c2::SettingResult* out) {
    out->failure = static_cast<aidl::android::hardware::media::c2::SettingResult::Failure>(in.failure);
    if (!translate(in.field, &out->field)) return false;
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: conflicts
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::WorkOrdinal& in, aidl::android::hardware::media::c2::WorkOrdinal* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.timestampUs > std::numeric_limits<int64_t>::max() || in.timestampUs < 0) {
        return false;
    }
    out->timestampUs = static_cast<int64_t>(in.timestampUs);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.frameIndex > std::numeric_limits<int64_t>::max() || in.frameIndex < 0) {
        return false;
    }
    out->frameIndex = static_cast<int64_t>(in.frameIndex);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.customOrdinal > std::numeric_limits<int64_t>::max() || in.customOrdinal < 0) {
        return false;
    }
    out->customOrdinal = static_cast<int64_t>(in.customOrdinal);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::BaseBlock& in, aidl::android::hardware::media::c2::BaseBlock* out) {
    switch (in.getDiscriminator()) {
        case ::android::hardware::media::c2::V1_0::BaseBlock::hidl_discriminator::nativeBlock:
            #error FIXME Unhandled type: handle
            break;
        case ::android::hardware::media::c2::V1_0::BaseBlock::hidl_discriminator::pooledBlock:
            // FIXME Unknown type: android.hardware.media.bufferpool@2.0::BufferStatusMessage
            // That type's package needs to be converted separately and the corresponding translate function should be added here.
            break;
        default:
            return false;
    }
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::Block& in, aidl::android::hardware::media::c2::Block* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.index > std::numeric_limits<int32_t>::max() || in.index < 0) {
        return false;
    }
    out->index = static_cast<int32_t>(in.index);
    {
        size_t size = in.meta.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.meta[i] > std::numeric_limits<int8_t>::max() || in.meta[i] < 0) {
                return false;
            }
            out->meta.push_back(static_cast<int8_t>(in.meta[i]));
        }
    }
    #error FIXME Unhandled type: handle
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::Buffer& in, aidl::android::hardware::media::c2::Buffer* out) {
    {
        size_t size = in.info.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.info[i] > std::numeric_limits<int8_t>::max() || in.info[i] < 0) {
                return false;
            }
            out->info.push_back(static_cast<int8_t>(in.info[i]));
        }
    }
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: blocks
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::InfoBuffer& in, aidl::android::hardware::media::c2::InfoBuffer* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.index > std::numeric_limits<int32_t>::max() || in.index < 0) {
        return false;
    }
    out->index = static_cast<int32_t>(in.index);
    if (!translate(in.buffer, &out->buffer)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FrameData& in, aidl::android::hardware::media::c2::FrameData* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.flags > std::numeric_limits<int32_t>::max() || in.flags < 0) {
        return false;
    }
    out->flags = static_cast<aidl::android::hardware::media::c2::FrameData::Flags>(in.flags);
    if (!translate(in.ordinal, &out->ordinal)) return false;
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: buffers
    {
        size_t size = in.configUpdate.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.configUpdate[i] > std::numeric_limits<int8_t>::max() || in.configUpdate[i] < 0) {
                return false;
            }
            out->configUpdate.push_back(static_cast<int8_t>(in.configUpdate[i]));
        }
    }
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: infoBuffers
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::Worklet& in, aidl::android::hardware::media::c2::Worklet* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.componentId > std::numeric_limits<int32_t>::max() || in.componentId < 0) {
        return false;
    }
    out->componentId = static_cast<int32_t>(in.componentId);
    {
        size_t size = in.tunings.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.tunings[i] > std::numeric_limits<int8_t>::max() || in.tunings[i] < 0) {
                return false;
            }
            out->tunings.push_back(static_cast<int8_t>(in.tunings[i]));
        }
    }
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: failures
    if (!translate(in.output, &out->output)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::Work& in, aidl::android::hardware::media::c2::Work* out) {
    {
        size_t size = in.chainInfo.size();
        for (size_t i = 0; i < size; i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.chainInfo[i] > std::numeric_limits<int8_t>::max() || in.chainInfo[i] < 0) {
                return false;
            }
            out->chainInfo.push_back(static_cast<int8_t>(in.chainInfo[i]));
        }
    }
    if (!translate(in.input, &out->input)) return false;
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: worklets
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.workletsProcessed > std::numeric_limits<int32_t>::max() || in.workletsProcessed < 0) {
        return false;
    }
    out->workletsProcessed = static_cast<int32_t>(in.workletsProcessed);
    out->result = static_cast<aidl::android::hardware::media::c2::Status>(in.result);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::WorkBundle& in, aidl::android::hardware::media::c2::WorkBundle* out) {
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: works
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: baseBlocks
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldSupportedValuesQuery& in, aidl::android::hardware::media::c2::FieldSupportedValuesQuery* out) {
    if (!translate(in.field, &out->field)) return false;
    out->type = static_cast<aidl::android::hardware::media::c2::FieldSupportedValuesQuery::Type>(in.type);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::FieldSupportedValuesQueryResult& in, aidl::android::hardware::media::c2::FieldSupportedValuesQueryResult* out) {
    out->status = static_cast<aidl::android::hardware::media::c2::Status>(in.status);
    if (!translate(in.values, &out->values)) return false;
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_2::SurfaceSyncObj& in, aidl::android::hardware::media::c2::SurfaceSyncObj* out) {
    #error FIXME Unhandled type: handle
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.bqId > std::numeric_limits<int64_t>::max() || in.bqId < 0) {
        return false;
    }
    out->bqId = static_cast<int64_t>(in.bqId);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.generationId > std::numeric_limits<int32_t>::max() || in.generationId < 0) {
        return false;
    }
    out->generationId = static_cast<int32_t>(in.generationId);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.consumerUsage > std::numeric_limits<int64_t>::max() || in.consumerUsage < 0) {
        return false;
    }
    out->consumerUsage = static_cast<int64_t>(in.consumerUsage);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::IComponentListener::RenderedFrame& in, aidl::android::hardware::media::c2::IComponentListener::RenderedFrame* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.bufferQueueId > std::numeric_limits<int64_t>::max() || in.bufferQueueId < 0) {
        return false;
    }
    out->bufferQueueId = static_cast<int64_t>(in.bufferQueueId);
    out->slotId = static_cast<int32_t>(in.slotId);
    out->timestampNs = static_cast<int64_t>(in.timestampNs);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::IComponentListener::InputBuffer& in, aidl::android::hardware::media::c2::IComponentListener::InputBuffer* out) {
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.frameIndex > std::numeric_limits<int64_t>::max() || in.frameIndex < 0) {
        return false;
    }
    out->frameIndex = static_cast<int64_t>(in.frameIndex);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.arrayIndex > std::numeric_limits<int32_t>::max() || in.arrayIndex < 0) {
        return false;
    }
    out->arrayIndex = static_cast<int32_t>(in.arrayIndex);
    return true;
}

__attribute__((warn_unused_result)) bool translate(const ::android::hardware::media::c2::V1_0::IComponentStore::ComponentTraits& in, aidl::android::hardware::media::c2::IComponentStore::ComponentTraits* out) {
    out->name = in.name;
    out->domain = static_cast<aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Domain>(in.domain);
    out->kind = static_cast<aidl::android::hardware::media::c2::IComponentStore::ComponentTraits::Kind>(in.kind);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.rank > std::numeric_limits<int32_t>::max() || in.rank < 0) {
        return false;
    }
    out->rank = static_cast<int32_t>(in.rank);
    out->mediaType = in.mediaType;
    {
        size_t size = in.aliases.size();
        for (size_t i = 0; i < size; i++) {
            out->aliases.push_back(in.aliases[i]);
        }
    }
    return true;
}

}  // namespace android::h2a