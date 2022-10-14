// FIXME: license file, or use the -l option to generate the files with the header.

// FIXME Remove this file if you don't need to translate types in this backend.

package android.hardware.media.c2;

public class Translate {
static public android.hardware.media.c2.FieldId h2aTranslate(android.hardware.media.c2.V1_0.FieldId in) {
    android.hardware.media.c2.FieldId out = new android.hardware.media.c2.FieldId();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.offset > 2147483647 || in.offset < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.offset");
    }
    out.offset = in.offset;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.size > 2147483647 || in.size < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.size");
    }
    out.size = in.size;
    return out;
}

static public android.hardware.media.c2.ParamField h2aTranslate(android.hardware.media.c2.V1_0.ParamField in) {
    android.hardware.media.c2.ParamField out = new android.hardware.media.c2.ParamField();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.index > 2147483647 || in.index < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.index");
    }
    out.index = in.index;
    out.fieldId = h2aTranslate(in.fieldId);
    return out;
}

static public android.hardware.media.c2.ParamDescriptor h2aTranslate(android.hardware.media.c2.V1_0.ParamDescriptor in) {
    android.hardware.media.c2.ParamDescriptor out = new android.hardware.media.c2.ParamDescriptor();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.index > 2147483647 || in.index < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.index");
    }
    out.index = in.index;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.attrib > 2147483647 || in.attrib < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.attrib");
    }
    out.attrib = in.attrib;
    out.name = in.name;
    if (in.dependencies != null) {
        out.dependencies = new int[in.dependencies.size()];
        for (int i = 0; i < in.dependencies.size(); i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.dependencies.get(i) > 2147483647 || in.dependencies.get(i) < 0) {
                throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.dependencies.get(i)");
            }
            out.dependencies[i] = in.dependencies.get(i);
        }
    }
    return out;
}

static public android.hardware.media.c2.ValueRange h2aTranslate(android.hardware.media.c2.V1_0.ValueRange in) {
    android.hardware.media.c2.ValueRange out = new android.hardware.media.c2.ValueRange();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.min > 9223372036854775807L || in.min < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.min");
    }
    out.min = in.min;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.max > 9223372036854775807L || in.max < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.max");
    }
    out.max = in.max;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.step > 9223372036854775807L || in.step < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.step");
    }
    out.step = in.step;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.num > 9223372036854775807L || in.num < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.num");
    }
    out.num = in.num;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.denom > 9223372036854775807L || in.denom < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.denom");
    }
    out.denom = in.denom;
    return out;
}

static public android.hardware.media.c2.FieldSupportedValues h2aTranslate(android.hardware.media.c2.V1_0.FieldSupportedValues in) {
    android.hardware.media.c2.FieldSupportedValues out = new android.hardware.media.c2.FieldSupportedValues();
    switch (in.getDiscriminator()) {
        case android.hardware.media.c2.V1_0.FieldSupportedValues.hidl_discriminator.empty:
            // Nothing to translate for Monostate.
            break;
        case android.hardware.media.c2.V1_0.FieldSupportedValues.hidl_discriminator.range:
            out.setRange(h2aTranslate(in.range()));
            break;
        case android.hardware.media.c2.V1_0.FieldSupportedValues.hidl_discriminator.values:
            if (in.values() != null) {
                out.setValues(new long[in.values().size()]);
                for (int i = 0; i < in.values().size(); i++) {
                    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
                    if (in.values().get(i) > 9223372036854775807L || in.values().get(i) < 0) {
                        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.values().get(i)");
                    }
                    out.getValues()[i] = in.values().get(i);
                }
            }
            break;
        case android.hardware.media.c2.V1_0.FieldSupportedValues.hidl_discriminator.flags:
            if (in.flags() != null) {
                out.setFlags(new long[in.flags().size()]);
                for (int i = 0; i < in.flags().size(); i++) {
                    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
                    if (in.flags().get(i) > 9223372036854775807L || in.flags().get(i) < 0) {
                        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.flags().get(i)");
                    }
                    out.getFlags()[i] = in.flags().get(i);
                }
            }
            break;
        default:
            throw new RuntimeException("Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
    }
    return out;
}

static public android.hardware.media.c2.ParamFieldValues h2aTranslate(android.hardware.media.c2.V1_0.ParamFieldValues in) {
    android.hardware.media.c2.ParamFieldValues out = new android.hardware.media.c2.ParamFieldValues();
    out.paramOrField = h2aTranslate(in.paramOrField);
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: values
    return out;
}

static public android.hardware.media.c2.FieldDescriptor h2aTranslate(android.hardware.media.c2.V1_0.FieldDescriptor in) {
    android.hardware.media.c2.FieldDescriptor out = new android.hardware.media.c2.FieldDescriptor();
    out.fieldId = h2aTranslate(in.fieldId);
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.type > 2147483647 || in.type < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.type");
    }
    out.type = in.type;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.structIndex > 2147483647 || in.structIndex < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.structIndex");
    }
    out.structIndex = in.structIndex;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.extent > 2147483647 || in.extent < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.extent");
    }
    out.extent = in.extent;
    out.name = in.name;
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: namedValues
    return out;
}

static public android.hardware.media.c2.FieldDescriptor.NamedValue h2aTranslate(android.hardware.media.c2.V1_0.FieldDescriptor.NamedValue in) {
    android.hardware.media.c2.FieldDescriptor.NamedValue out = new android.hardware.media.c2.FieldDescriptor.NamedValue();
    out.name = in.name;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.value > 9223372036854775807L || in.value < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.value");
    }
    out.value = in.value;
    return out;
}

static public android.hardware.media.c2.StructDescriptor h2aTranslate(android.hardware.media.c2.V1_0.StructDescriptor in) {
    android.hardware.media.c2.StructDescriptor out = new android.hardware.media.c2.StructDescriptor();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.type > 2147483647 || in.type < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.type");
    }
    out.type = in.type;
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: fields
    return out;
}

static public android.hardware.media.c2.SettingResult h2aTranslate(android.hardware.media.c2.V1_0.SettingResult in) {
    android.hardware.media.c2.SettingResult out = new android.hardware.media.c2.SettingResult();
    out.failure = in.failure;
    out.field = h2aTranslate(in.field);
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: conflicts
    return out;
}

static public android.hardware.media.c2.WorkOrdinal h2aTranslate(android.hardware.media.c2.V1_0.WorkOrdinal in) {
    android.hardware.media.c2.WorkOrdinal out = new android.hardware.media.c2.WorkOrdinal();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.timestampUs > 9223372036854775807L || in.timestampUs < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.timestampUs");
    }
    out.timestampUs = in.timestampUs;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.frameIndex > 9223372036854775807L || in.frameIndex < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.frameIndex");
    }
    out.frameIndex = in.frameIndex;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.customOrdinal > 9223372036854775807L || in.customOrdinal < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.customOrdinal");
    }
    out.customOrdinal = in.customOrdinal;
    return out;
}

static public android.hardware.media.c2.BaseBlock h2aTranslate(android.hardware.media.c2.V1_0.BaseBlock in) {
    android.hardware.media.c2.BaseBlock out = new android.hardware.media.c2.BaseBlock();
    switch (in.getDiscriminator()) {
        case android.hardware.media.c2.V1_0.BaseBlock.hidl_discriminator.nativeBlock:
            #error FIXME Unhandled type: handle
            break;
        case android.hardware.media.c2.V1_0.BaseBlock.hidl_discriminator.pooledBlock:
            // FIXME Unknown type: android.hardware.media.bufferpool@2.0::BufferStatusMessage
            // That type's package needs to be converted separately and the corresponding translate function should be added here.
            break;
        default:
            throw new RuntimeException("Unknown discriminator value: " + Integer.toString(in.getDiscriminator()));
    }
    return out;
}

static public android.hardware.media.c2.Block h2aTranslate(android.hardware.media.c2.V1_0.Block in) {
    android.hardware.media.c2.Block out = new android.hardware.media.c2.Block();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.index > 2147483647 || in.index < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.index");
    }
    out.index = in.index;
    if (in.meta != null) {
        out.meta = new byte[in.meta.size()];
        for (int i = 0; i < in.meta.size(); i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.meta.get(i) > 127 || in.meta.get(i) < 0) {
                throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.meta.get(i)");
            }
            out.meta[i] = in.meta.get(i);
        }
    }
    #error FIXME Unhandled type: handle
    return out;
}

static public android.hardware.media.c2.Buffer h2aTranslate(android.hardware.media.c2.V1_0.Buffer in) {
    android.hardware.media.c2.Buffer out = new android.hardware.media.c2.Buffer();
    if (in.info != null) {
        out.info = new byte[in.info.size()];
        for (int i = 0; i < in.info.size(); i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.info.get(i) > 127 || in.info.get(i) < 0) {
                throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.info.get(i)");
            }
            out.info[i] = in.info.get(i);
        }
    }
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: blocks
    return out;
}

static public android.hardware.media.c2.InfoBuffer h2aTranslate(android.hardware.media.c2.V1_0.InfoBuffer in) {
    android.hardware.media.c2.InfoBuffer out = new android.hardware.media.c2.InfoBuffer();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.index > 2147483647 || in.index < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.index");
    }
    out.index = in.index;
    out.buffer = h2aTranslate(in.buffer);
    return out;
}

static public android.hardware.media.c2.FrameData h2aTranslate(android.hardware.media.c2.V1_0.FrameData in) {
    android.hardware.media.c2.FrameData out = new android.hardware.media.c2.FrameData();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.flags > 2147483647 || in.flags < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.flags");
    }
    out.flags = in.flags;
    out.ordinal = h2aTranslate(in.ordinal);
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: buffers
    if (in.configUpdate != null) {
        out.configUpdate = new byte[in.configUpdate.size()];
        for (int i = 0; i < in.configUpdate.size(); i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.configUpdate.get(i) > 127 || in.configUpdate.get(i) < 0) {
                throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.configUpdate.get(i)");
            }
            out.configUpdate[i] = in.configUpdate.get(i);
        }
    }
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: infoBuffers
    return out;
}

static public android.hardware.media.c2.Worklet h2aTranslate(android.hardware.media.c2.V1_0.Worklet in) {
    android.hardware.media.c2.Worklet out = new android.hardware.media.c2.Worklet();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.componentId > 2147483647 || in.componentId < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.componentId");
    }
    out.componentId = in.componentId;
    if (in.tunings != null) {
        out.tunings = new byte[in.tunings.size()];
        for (int i = 0; i < in.tunings.size(); i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.tunings.get(i) > 127 || in.tunings.get(i) < 0) {
                throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.tunings.get(i)");
            }
            out.tunings[i] = in.tunings.get(i);
        }
    }
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: failures
    out.output = h2aTranslate(in.output);
    return out;
}

static public android.hardware.media.c2.Work h2aTranslate(android.hardware.media.c2.V1_0.Work in) {
    android.hardware.media.c2.Work out = new android.hardware.media.c2.Work();
    if (in.chainInfo != null) {
        out.chainInfo = new byte[in.chainInfo.size()];
        for (int i = 0; i < in.chainInfo.size(); i++) {
            // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
            if (in.chainInfo.get(i) > 127 || in.chainInfo.get(i) < 0) {
                throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.chainInfo.get(i)");
            }
            out.chainInfo[i] = in.chainInfo.get(i);
        }
    }
    out.input = h2aTranslate(in.input);
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: worklets
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.workletsProcessed > 2147483647 || in.workletsProcessed < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.workletsProcessed");
    }
    out.workletsProcessed = in.workletsProcessed;
    out.result = in.result;
    return out;
}

static public android.hardware.media.c2.WorkBundle h2aTranslate(android.hardware.media.c2.V1_0.WorkBundle in) {
    android.hardware.media.c2.WorkBundle out = new android.hardware.media.c2.WorkBundle();
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: works
    #error Arrays of NamedTypes are not currently not supported. Needs implementation for field: baseBlocks
    return out;
}

static public android.hardware.media.c2.FieldSupportedValuesQuery h2aTranslate(android.hardware.media.c2.V1_0.FieldSupportedValuesQuery in) {
    android.hardware.media.c2.FieldSupportedValuesQuery out = new android.hardware.media.c2.FieldSupportedValuesQuery();
    out.field = h2aTranslate(in.field);
    out.type = in.type;
    return out;
}

static public android.hardware.media.c2.FieldSupportedValuesQueryResult h2aTranslate(android.hardware.media.c2.V1_0.FieldSupportedValuesQueryResult in) {
    android.hardware.media.c2.FieldSupportedValuesQueryResult out = new android.hardware.media.c2.FieldSupportedValuesQueryResult();
    out.status = in.status;
    out.values = h2aTranslate(in.values);
    return out;
}

static public android.hardware.media.c2.SurfaceSyncObj h2aTranslate(android.hardware.media.c2.V1_2.SurfaceSyncObj in) {
    android.hardware.media.c2.SurfaceSyncObj out = new android.hardware.media.c2.SurfaceSyncObj();
    #error FIXME Unhandled type: handle
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.bqId > 9223372036854775807L || in.bqId < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.bqId");
    }
    out.bqId = in.bqId;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.generationId > 2147483647 || in.generationId < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.generationId");
    }
    out.generationId = in.generationId;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.consumerUsage > 9223372036854775807L || in.consumerUsage < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.consumerUsage");
    }
    out.consumerUsage = in.consumerUsage;
    return out;
}

static public android.hardware.media.c2.IComponentListener.RenderedFrame h2aTranslate(android.hardware.media.c2.V1_0.IComponentListener.RenderedFrame in) {
    android.hardware.media.c2.IComponentListener.RenderedFrame out = new android.hardware.media.c2.IComponentListener.RenderedFrame();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.bufferQueueId > 9223372036854775807L || in.bufferQueueId < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.bufferQueueId");
    }
    out.bufferQueueId = in.bufferQueueId;
    out.slotId = in.slotId;
    out.timestampNs = in.timestampNs;
    return out;
}

static public android.hardware.media.c2.IComponentListener.InputBuffer h2aTranslate(android.hardware.media.c2.V1_0.IComponentListener.InputBuffer in) {
    android.hardware.media.c2.IComponentListener.InputBuffer out = new android.hardware.media.c2.IComponentListener.InputBuffer();
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.frameIndex > 9223372036854775807L || in.frameIndex < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.frameIndex");
    }
    out.frameIndex = in.frameIndex;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.arrayIndex > 2147483647 || in.arrayIndex < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.arrayIndex");
    }
    out.arrayIndex = in.arrayIndex;
    return out;
}

static public android.hardware.media.c2.IComponentStore.ComponentTraits h2aTranslate(android.hardware.media.c2.V1_0.IComponentStore.ComponentTraits in) {
    android.hardware.media.c2.IComponentStore.ComponentTraits out = new android.hardware.media.c2.IComponentStore.ComponentTraits();
    out.name = in.name;
    out.domain = in.domain;
    out.kind = in.kind;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.rank > 2147483647 || in.rank < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.rank");
    }
    out.rank = in.rank;
    out.mediaType = in.mediaType;
    if (in.aliases != null) {
        out.aliases = new String[in.aliases.size()];
        for (int i = 0; i < in.aliases.size(); i++) {
            out.aliases[i] = in.aliases.get(i);
        }
    }
    return out;
}

}