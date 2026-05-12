/**
 *******************************************************************************
 * @file    application_base.cpp
 * @brief   Zephyr thread/application abstraction implementation
 *******************************************************************************
 */

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "platform/zephyr/application_base.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/time_units.h>

namespace omnigen {

LOG_MODULE_REGISTER(application_base, CONFIG_LOG_DEFAULT_LEVEL);

/* ------- static members -------------------------------------------------------------------------------------------*/

StaticAppBase::TaskInfo StaticAppBase::registry_[StaticAppBase::kMaxApplications] = {};
size_t StaticAppBase::registryCount_                                              = 0U;
uint8_t StaticAppBase::initedCount_                                               = 0U;

/* ------- helper ----------------------------------------------------------------------------------------------------*/

static inline uint32_t saturateToU32(size_t value) {
    return (value > static_cast<size_t>(UINT32_MAX)) ? UINT32_MAX : static_cast<uint32_t>(value);
}

/* ------- StaticAppBase --------------------------------------------------------------------------------------------*/

StaticAppBase::StaticAppBase(bool enable, const char* name, size_t stackSize, k_thread_stack_t* stackBuf, int priority)
    : taskInfo_{enable, name, stackSize, this, priority, &taskHandle_, &thread_, stackBuf} {
    (void)k_sem_init(&initSem_, 0U, 1U);
    (void)registerTaskInfo(taskInfo_);
}

bool StaticAppBase::registerTaskInfo(const TaskInfo& taskInfo) {
    if (registryCount_ >= static_cast<size_t>(kMaxApplications)) {
        return false;
    }

    registry_[registryCount_] = taskInfo;
    registryCount_++;
    return true;
}

void StaticAppBase::startApplications() {
    LOG_INF("Starting %u registered application threads", static_cast<unsigned int>(registryCount_));

    for (size_t i = 0; i < registryCount_; ++i) {
        TaskInfo& app = registry_[i];
        if (!app.enable || app.app == nullptr || app.threadObj == nullptr || app.stackBuf == nullptr ||
            app.taskHandle == nullptr) {
            LOG_WRN("Skip app[%u]: enable=%u app=%p stack=%p handle=%p", static_cast<unsigned int>(i),
                    app.enable ? 1U : 0U, static_cast<void*>(app.app), static_cast<void*>(app.stackBuf),
                    static_cast<void*>(app.taskHandle));
            continue;
        }

        *(app.taskHandle) = k_thread_create(app.threadObj, app.stackBuf, app.stackSize, &StaticAppBase::taskEntry,
                                            app.app, nullptr, nullptr, app.priority, 0U, K_NO_WAIT);

        (void)k_thread_name_set(*(app.taskHandle), app.name);
        LOG_INF("Started app[%u]: name=%s prio=%d stack=%u tid=%p", static_cast<unsigned int>(i),
                (app.name != nullptr) ? app.name : "(null)", app.priority, static_cast<unsigned int>(app.stackSize),
                static_cast<void*>(*(app.taskHandle)));
    }
}

void StaticAppBase::initEvent() { k_sem_give(&initSem_); }

void StaticAppBase::waitInit() { (void)k_sem_take(&initSem_, K_FOREVER); }

const char* StaticAppBase::getName() const { return taskInfo_.name; }

uint32_t StaticAppBase::getStackHighWaterMark() const {
    if (taskHandle_ == nullptr) {
        return 0U;
    }

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
    size_t unused = 0U;
    if (k_thread_stack_space_get(taskHandle_, &unused) == 0) {
        return saturateToU32(unused);
    }
#endif

    return 0U;
}

k_tid_t StaticAppBase::getTaskHandle() const { return taskHandle_; }

float StaticAppBase::getRunTime() const { return runTimeUs_; }

float StaticAppBase::getMaxRunTime() const { return maxRunTimeUs_; }

void StaticAppBase::executeRun() {
#if defined(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)
    const uint64_t startCycles = k_cycle_get_64();
    run();
    const uint64_t endCycles     = k_cycle_get_64();
    const uint64_t elapsedCycles = endCycles - startCycles;
    runTimeUs_                   = static_cast<float>(k_cyc_to_us_floor64(elapsedCycles));
#else
    const uint32_t startCycles = k_cycle_get_32();
    run();
    const uint32_t endCycles     = k_cycle_get_32();
    const uint32_t elapsedCycles = endCycles - startCycles;
    runTimeUs_                   = static_cast<float>(k_cyc_to_us_floor32(elapsedCycles));
#endif

    if (runTimeUs_ > maxRunTimeUs_) {
        maxRunTimeUs_ = runTimeUs_;
    }
}

void StaticAppBase::taskEntry(void* p1, void* p2, void* p3) {
    (void)p2;
    (void)p3;

    IApplication* app = static_cast<IApplication*>(p1);
    if (app == nullptr) {
        return;
    }

    app->init();
    app->initEvent();
    initedCount_++;

    app->taskLoop();
}

/* ------- PeriodicApp ----------------------------------------------------------------------------------------------*/

PeriodicApp::PeriodicApp(bool enable, const char* name, size_t stackSize, k_thread_stack_t* stackBuf, int priority,
                         uint32_t periodMs)
    : StaticAppBase(enable, name, stackSize, stackBuf, priority), periodMs_(periodMs) {}

void PeriodicApp::taskLoop() {
    int64_t nextWake = k_uptime_get();

    while (true) {
        executeRun();

        nextWake += static_cast<int64_t>(periodMs_);
        const int64_t now    = k_uptime_get();
        const int64_t waitMs = nextWake - now;
        if (waitMs > 0) {
            (void)k_sleep(K_MSEC(waitMs));
        } else {
            k_yield();
        }
    }
}

/* ------- ContinuousApp --------------------------------------------------------------------------------------------*/

void ContinuousApp::taskLoop() {
    while (true) {
        executeRun();
    }
}

/* ------- NotifyApp -------------------------------------------------------------------------------------------------*/

NotifyApp::NotifyApp(bool enable, const char* name, size_t stackSize, k_thread_stack_t* stackBuf, int priority,
                     k_timeout_t timeout)
    : StaticAppBase(enable, name, stackSize, stackBuf, priority), timeout_(timeout) {
    (void)k_sem_init(&notifySem_, 0U, K_SEM_MAX_LIMIT);
}

void NotifyApp::notify() { k_sem_give(&notifySem_); }

void NotifyApp::notifyFromISR() { k_sem_give(&notifySem_); }

void NotifyApp::taskLoop() {
    while (true) {
        (void)k_sem_take(&notifySem_, timeout_);
        executeRun();
    }
}

/* ------- QueueApp --------------------------------------------------------------------------------------------------*/

QueueApp::QueueApp(bool enable, const char* name, size_t stackSize, k_thread_stack_t* stackBuf, int priority,
                   uint16_t msgQueueSize, uint8_t* queueBuf, k_timeout_t timeout)
    : StaticAppBase(enable, name, stackSize, stackBuf, priority), timeout_(timeout) {
    if (queueBuf != nullptr && msgQueueSize > 0U) {
        k_msgq_init(&commQueue_, reinterpret_cast<char*>(queueBuf), sizeof(IPCMsg), msgQueueSize);
        queueReady_ = true;
    }
}

int QueueApp::sendMsg(void* pMsg, uint16_t msgLen, k_timeout_t timeout) {
    if (!queueReady_) {
        return -EINVAL;
    }

    const IPCMsg msg(pMsg, msgLen);
    return k_msgq_put(&commQueue_, &msg, timeout);
}

int QueueApp::sendMsgFromISR(void* pMsg, uint16_t msgLen) {
    if (!queueReady_) {
        return -EINVAL;
    }

    const IPCMsg msg(pMsg, msgLen);
    return k_msgq_put(&commQueue_, &msg, K_NO_WAIT);
}

const QueueApp::IPCMsg& QueueApp::getCurrentMsg() const { return currentMsg_; }

void QueueApp::taskLoop() {
    while (true) {
        if (!queueReady_) {
            currentMsg_.pMsg   = nullptr;
            currentMsg_.msgLen = 0U;
            executeRun();
            k_yield();
            continue;
        }

        const int rc = k_msgq_get(&commQueue_, &currentMsg_, timeout_);
        if (rc == 0) {
            executeRun();
        } else {
            currentMsg_.pMsg   = nullptr;
            currentMsg_.msgLen = 0U;
            executeRun();
        }
    }
}

} // namespace omnigen
