# OmniGen H7 Architecture UML

This document records the current OmniGen H7 architecture UML. The PNG is generated from a local drawing script, while the Mermaid source is kept below for maintenance.

## PNG

![OmniGen H7 Architecture UML](architecture_uml.png)

## 1. Core Class Diagram

```mermaid
classDiagram
    class SystemContext {
        +MockWaveSink wave_sink
        +ZephyrStorage storage
        +ZephyrDisplay display
        +ZephyrFilterSwitch filter_switch
        +SignalEngine signal_engine
        +DirectCommandBus command_bus
        +DirectRequestBus request_bus
    }

    class SignalEngine {
        -WaveSinkPort& sink_
        -SignalEngineState state_
        -SignalProfile active_profile_
        -SignalProfile pending_profile_
        -SignalLimits limits_
        -WaveformId selected_waveform_
        -ErrorCode last_fault_
        -uint32_t command_count_
        +handle_command(SignalCommand) Result~void~
        +snapshot() SignalEngineSnapshot
    }

    class CommandBusPort {
        <<interface>>
        +submit(AppCommand) Result~void~
    }

    class RequestBusPort {
        <<interface>>
        +request(AppRequest, AppResponse) Result~void~
    }

    class DirectCommandBus {
        -SignalEngine& signal_engine_
        -FilterSwitchPort& filter_switch_
        +submit(AppCommand) Result~void~
    }

    class DirectRequestBus {
        -SignalEngine& signal_engine_
        -FilterSwitchPort& filter_switch_
        -StoragePort& storage_
        -DisplayPort& display_
        -bool& system_ready_
        +request(AppRequest, AppResponse) Result~void~
    }

    class WaveSinkPort {
        <<interface>>
        +configure(SignalProfile) Result~void~
        +start() Result~void~
        +stop() Result~void~
        +submit_block(WaveSampleBlock) Result~void~
    }

    class StoragePort {
        <<interface>>
        +mount() Result~void~
        +read(StorageReadRequest) Result~void~
        +write(StorageWriteRequest) Result~void~
        +erase_sector(uint32_t) Result~void~
        +erase_range(StorageEraseRangeRequest) Result~void~
        +mounted() bool
    }

    class DisplayPort {
        <<interface>>
        +mount() Result~void~
        +clear(uint16_t) Result~void~
        +fill(DisplayRect, uint16_t) Result~void~
        +blit(DisplayBlitRequest) Result~void~
        +mounted() bool
    }

    class FilterSwitchPort {
        <<interface>>
        +mount() Result~void~
        +set_mode(FilterMode) Result~void~
        +set_mute(bool) Result~void~
        +mounted() bool
        +muted() bool
        +mode() FilterMode
    }

    class MockWaveSink
    class ZephyrWaveSink
    class ZephyrStorage
    class ZephyrDisplay
    class ZephyrFilterSwitch

    SystemContext *-- MockWaveSink
    SystemContext *-- ZephyrStorage
    SystemContext *-- ZephyrDisplay
    SystemContext *-- ZephyrFilterSwitch
    SystemContext *-- SignalEngine
    SystemContext *-- DirectCommandBus
    SystemContext *-- DirectRequestBus

    CommandBusPort <|.. DirectCommandBus
    RequestBusPort <|.. DirectRequestBus
    WaveSinkPort <|.. MockWaveSink
    WaveSinkPort <|.. ZephyrWaveSink
    StoragePort <|.. ZephyrStorage
    DisplayPort <|.. ZephyrDisplay
    FilterSwitchPort <|.. ZephyrFilterSwitch

    SignalEngine --> WaveSinkPort
    DirectCommandBus --> SignalEngine
    DirectCommandBus --> FilterSwitchPort
    DirectRequestBus --> SignalEngine
    DirectRequestBus --> FilterSwitchPort
    DirectRequestBus --> StoragePort
    DirectRequestBus --> DisplayPort
```

## 2. Data Bus Flow

```mermaid
flowchart LR
    Shell[Shell / UI / Host] -->|write AppCommand| CommandBusPort
    CommandBusPort --> DirectCommandBus
    DirectCommandBus -->|SignalCommand| SignalEngine
    DirectCommandBus -->|FilterCommand| FilterSwitchPort

    Shell -->|read AppRequest| RequestBusPort
    RequestBusPort --> DirectRequestBus
    DirectRequestBus -->|SignalSnapshot| SignalEngine
    DirectRequestBus -->|FilterStatus| FilterSwitchPort
    DirectRequestBus -->|StorageStatus| StoragePort
    DirectRequestBus -->|DisplayStatus| DisplayPort
    DirectRequestBus -->|SystemStatus| SystemReady[system_ready]

    SignalEngine -->|current sink| MockWaveSink
    ZephyrWaveSink -. available adapter .-> WaveSinkPort
    StoragePort --> ZephyrStorage
    DisplayPort --> ZephyrDisplay
    FilterSwitchPort --> ZephyrFilterSwitch

    ZephyrWaveSink --> DACDriver[dac_wave_sink.c]
    ZephyrStorage --> W25Q64Driver[w25q64_support.c]
    ZephyrDisplay --> ILI9481Driver[ili9481_support.c]
```

## 3. Data Type Diagram

```mermaid
classDiagram
    class AppCommand {
        +AppCommandKind kind
        +SignalCommand signal
        +FilterCommand filter
    }

    class SignalCommand {
        +SignalCommandKind kind
        +CommandSource source
        +uint32_t sequence
        +FrequencyHz frequency
        +VoltageMv amplitude
        +VoltageMv offset
        +WaveformKind waveform
        +DutyPermille duty
        +uint32_t preset_id
    }

    class FilterCommand {
        +FilterCommandKind kind
        +CommandSource source
        +uint32_t sequence
        +FilterMode mode
        +bool mute
    }

    class AppRequest {
        +AppRequestKind kind
        +uint32_t sequence
    }

    class AppResponse {
        +AppRequestKind kind
        +ErrorCode status
        +SignalEngineSnapshot signal
        +FilterStatus filter
        +StorageStatus storage
        +DisplayStatus display
        +SystemStatus system
    }

    class SignalEngineSnapshot
    class FilterStatus
    class StorageStatus
    class DisplayStatus
    class SystemStatus
    class WaveSampleBlock

    AppCommand *-- SignalCommand
    AppCommand *-- FilterCommand
    AppResponse *-- SignalEngineSnapshot
    AppResponse *-- FilterStatus
    AppResponse *-- StorageStatus
    AppResponse *-- DisplayStatus
    AppResponse *-- SystemStatus
```
