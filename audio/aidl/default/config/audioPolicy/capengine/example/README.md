# Migration command lines

- Phone example command line

> cd $ANDROID_BUILD_TOP

> python3 legacyToAidlCapEngineConfigConversion.py
> --legacyEngineStrategyConfiguration ../config/example/phone/audio_policy_engine_product_strategies.xml
> --legacyPFwEdds ../parameter-framework/examples/Phone/Settings/device_for_product_strategy_accessibility.pfw
>                 ../parameter-framework/examples/Phone/Settings/device_for_product_strategy_dtmf.pfw
>                 ../parameter-framework/examples/Phone/Settings/device_for_product_strategy_enforced_audible.pfw
>                 ../parameter-framework/examples/Phone/Settings/device_for_product_strategy_media.pfw
>                 ../parameter-framework/examples/Phone/Settings/device_for_product_strategy_patch.pfw
>                 ../parameter-framework/examples/Phone/Settings/device_for_product_strategy_phone.pfw
>                 ../parameter-framework/examples/Phone/Settings/device_for_product_strategy_rerouting.pfw
>                 ../parameter-framework/examples/Phone/Settings/device_for_product_strategy_sonification.pfw
>                 ../parameter-framework/examples/Phone/Settings/device_for_product_strategy_sonification_respectful.pfw
>                 ../parameter-framework/examples/Phone/Settings/device_for_product_strategy_transmitted_through_speaker.pfw
> --outputDir $ANDROID_BUILD_TOP/hardware/interfaces/audio/aidl/default/config/audioPolicy/capengine/example/phone/
> --androidTreeRoot $ANDROID_BUILD_TOP


- Automotive example command line

> python3 legacyToAidlCapEngineConfigConversion.py
> --legacyEngineStrategyConfiguration ../config/example/automotive/audio_policy_engine_product_strategies.xml
> --legacyPFwEdds ../parameter-framework/examples/Car/Settings/device_for_product_strategies.pfw
>                 ../parameter-framework/examples/Settings/device_for_input_source.pfw
> --outputDir $ANDROID_BUILD_TOP/hardware/interfaces/audio/aidl/default/config/audioPolicy/capengine/example/automotive/
> --androidTreeRoot $ANDROID_BUILD_TOP
