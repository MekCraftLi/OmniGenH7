/**
 *******************************************************************************
 * @file    application_base.h
 * @brief   Zephyr thread/application abstraction base classes
 *******************************************************************************
 * @attention
 *
 * This file ports docs/application-base.h into Zephyr primitives while
 * keeping class names and usage patterns familiar:
 * - IApplication
 * - StaticAppBase
 * - PeriodicApp / ContinuousApp / NotifyApp / QueueApp
 *
 *******************************************************************************
 * @note
 *
 * Thread stack memory is still provided by caller, usually with
 * K_THREAD_STACK_DEFINE(...), and passed as k_thread_stack_t*.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/5/12
 * @version 1.0
 *******************************************************************************
 */

/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef OMNIGEN_H7_APP_PLATFORM_ZEPHYR_APPLICATION_BASE_H
#define OMNIGEN_H7_APP_PLATFORM_ZEPHYR_APPLICATION_BASE_H

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <zephyr/kernel.h>

#include <cstddef>
#include <cstdint>

namespace omnigen {

/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class IApplication {
public:
    virtual ~IApplication() = default;

    virtual void init() = 0;
    virtual void run() = 0;

    /* Task entry loop strategy implemented by derived class. */
    virtual void taskLoop() = 0;

    [[nodiscard]] virtual const char* getName() const = 0;
    [[nodiscard]] virtual float getRunTime() const = 0;
    [[nodiscard]] virtual uint32_t getStackHighWaterMark() const = 0;
    [[nodiscard]] virtual k_tid_t getTaskHandle() const = 0;

    virtual void initEvent() = 0;
    virtual void waitInit() = 0;
};

class StaticAppBase : public IApplication {
protected:
    struct TaskInfo {
        bool enable;
        const char* name;
        size_t stackSize;
        IApplication* app;
        int priority;
        k_tid_t* taskHandle;
        struct k_thread* threadObj;
        k_thread_stack_t* stackBuf;
    };

public:
    StaticAppBase(bool enable, const char* name, size_t stackSize, k_thread_stack_t* stackBuf, int priority);
    ~StaticAppBase() override = default;

    static void startApplications();

    void initEvent() override;
    void waitInit() override;

    [[nodiscard]] const char* getName() const override;
    [[nodiscard]] uint32_t getStackHighWaterMark() const override;
    [[nodiscard]] k_tid_t getTaskHandle() const override;
    [[nodiscard]] float getRunTime() const override;
    [[nodiscard]] float getMaxRunTime() const;

protected:
    void executeRun();

private:
    enum { kMaxApplications = 16 };

    static bool registerTaskInfo(const TaskInfo& taskInfo);
    static void taskEntry(void* p1, void* p2, void* p3);

protected:
    struct k_sem initSem_{};
    k_tid_t taskHandle_{nullptr};
    struct k_thread thread_{};
    float runTimeUs_{0.0f};
    float maxRunTimeUs_{0.0f};
    TaskInfo taskInfo_{};

private:
    static TaskInfo registry_[kMaxApplications];
    static size_t registryCount_;
    static uint8_t initedCount_;
};

class PeriodicApp : public StaticAppBase {
public:
    PeriodicApp(bool enable, const char* name, size_t stackSize, k_thread_stack_t* stackBuf, int priority,
                uint32_t periodMs);

protected:
    void taskLoop() override;

private:
    uint32_t periodMs_{0U};
};

class ContinuousApp : public StaticAppBase {
public:
    using StaticAppBase::StaticAppBase;

protected:
    void taskLoop() override;
};

class NotifyApp : public StaticAppBase {
public:
    NotifyApp(bool enable, const char* name, size_t stackSize, k_thread_stack_t* stackBuf, int priority,
              k_timeout_t timeout = K_FOREVER);

    void notify();
    void notifyFromISR();

protected:
    void taskLoop() override;

private:
    k_timeout_t timeout_;
    struct k_sem notifySem_{};
};

class QueueApp : public StaticAppBase {
public:
    class IPCMsg {
    public:
        IPCMsg() : pMsg(nullptr), msgLen(0) {}
        IPCMsg(void* msg, uint16_t len) : pMsg(msg), msgLen(len) {}
        void* pMsg;
        uint16_t msgLen;
    };

    QueueApp(bool enable, const char* name, size_t stackSize, k_thread_stack_t* stackBuf, int priority,
             uint16_t msgQueueSize, uint8_t* queueBuf, k_timeout_t timeout = K_FOREVER);

    int sendMsg(void* pMsg, uint16_t msgLen, k_timeout_t timeout = K_NO_WAIT);
    int sendMsgFromISR(void* pMsg, uint16_t msgLen);

    const IPCMsg& getCurrentMsg() const;

protected:
    void taskLoop() override;

private:
    struct k_msgq commQueue_{};
    bool queueReady_{false};
    k_timeout_t timeout_;
    IPCMsg currentMsg_{};
};

} // namespace omnigen

#endif /* OMNIGEN_H7_APP_PLATFORM_ZEPHYR_APPLICATION_BASE_H */
