/*
 * cmsis.h
 *
 *  Created on: 20Feb.,2017
 *      Author: podonoghue
 */
#ifndef PROJECT_HEADERS_CMSIS_H_
#define PROJECT_HEADERS_CMSIS_H_

#include "cmsis_os.h"
#include "hardware.h"

namespace CMSIS {

/**
 * Wrapper for CMSIS Timer
 */
class Timer {

private:
   uint32_t             os_timer_cb[6] = { 0 };
   const osTimerDef_t   os_timer_def;

public:
   /**
    * Create timer
    *
    * @param callback Timer callback function
    * @param type     Type of timer i.e. osTimerOnce, osTimerPeriodic
    * @param argument Pointer to data to pass to callback
    *
    * @code
    *  timer = new CMSIS::Timer(checkCaseTemp, osTimerPeriodic);
    *  timer->start(1000);
    * @endcode
    */
   Timer(
         void           (*callback)(const void *),
         os_timer_type  type,
         void           *argument=nullptr) :
            os_timer_def {callback, os_timer_cb}
   {
      osTimerId timer_id = osTimerCreate(&os_timer_def, type, argument);
      assert((void*)timer_id == (void*)os_timer_cb);
   }
   /**
    * Delete timer
    */
   ~Timer() {
      osTimerDelete((osTimerId)os_timer_cb);
   }
   /**
    * Stop the timer
    */
   void stop() {
      osTimerStop((osTimerId)os_timer_cb);
   }
   /**
    * Start or restart timer
    *
    * @param millisec Interval in milliseconds
    */
   void start(int millisec) {
      USBDM::setAndCheckCmsisErrorCode(osTimerStart((osTimerId)os_timer_cb, millisec));
   }
   /**
    * Start or restart timer
    *
    * @param interval Interval in seconds
    */
   void start(double interval) {
      start((int)round(interval*1000.0));
   }
};

/**
 * Wrapper for CMSIS Mutex
 */
class Mutex {

private:
   uint32_t           os_mutex_cb[4] = { 0 };
   const osMutexDef_t os_mutex_def   = {os_mutex_cb};

public:
   /**
    * Create mutex
    */
   Mutex() {
      osMutexId mutex_id = osMutexCreate(&os_mutex_def);
      assert((void*)mutex_id == (void*)os_mutex_cb);
   }
   /**
    * Delete mutex
    */
   ~Mutex() {
      osMutexDelete((osMutexId) os_mutex_cb);
   }
   /**
    * Obtain mutex
    *
    * @param millisec How long to wait in milliseconds. Use osWaitForever for indefinite wait
    */
   void wait(uint32_t millisec=osWaitForever) {
      osMutexWait((osMutexId) os_mutex_cb, millisec);
   }
   /**
    * Release mutex
    */
   void release() {
      osMutexRelease((osMutexId) os_mutex_cb);
   }
};
/**
 * Wrapper for CMSIS Semaphore
 */
class Semaphore {

private:
   uint32_t               os_semaphore_cb[2] = { 0 };
   const osSemaphoreDef_t os_semaphore_def   = { os_semaphore_cb };

public:
   /**
    * Create semaphore
    */
   Semaphore(int32_t count) {
      osSemaphoreId semaphore_id = osSemaphoreCreate(&os_semaphore_def, count);
      assert((void*)semaphore_id == (void*)os_semaphore_cb);
   }
   /**
    * Delete semaphore
    */
   ~Semaphore() {
      osSemaphoreDelete((osSemaphoreId)os_semaphore_cb);
   }
   /**
    * Obtain semaphore
    *
    * @param millisec How long to wait in milliseconds. Use osWaitForever for indefinite wait
    */
   int32_t wait(uint32_t millisec=osWaitForever) {
      int32_t rc = osSemaphoreWait((osSemaphoreId)os_semaphore_cb, millisec);
      assert(rc >= 0);
      return rc;
   }
   /**
    * Release semaphore
    */
   void release() {
      osStatus status = osSemaphoreRelease((osSemaphoreId)os_semaphore_cb);
      assert(status == osOK);
   }
};
/**
 * Wrapper for CMSIS Memory Allocation Pool
 *
 * @tparam T      Type of items in pool
 * @tparam size   Size of pool in items
 *
 * Example:
 * @code
 *  struct Data {
 *     int a;
 *     int b;
 *  };
 *
 *  CMSIS::Pool<Data, 20> *pool = new CMSIS::Pool<Data, 20>;
 *
 *  Data *ar[20] = {0};
 *  for (unsigned i=0; i<(sizeof(ar)/sizeof(ar[0])); i++) {
 *     ar[i] = pool->alloc();
 *     if (ar[i] == nullptr) {
 *        break;
 *     }
 *     ar[i]->a = i;
 *     ar[i]->b = i*i;
 *  }
 *  for (unsigned i=0; i<(sizeof(ar)/sizeof(ar[0])); i++) {
 *     if (ar[i] != nullptr) {
 *        pool->free(ar[i]);
 *     }
 *  }
 * @endcode
 */
template <typename T, size_t size>
class Pool {

private:
   static uint32_t pool[3+((sizeof(T)+3)/4)*size];
   const osPoolDef_t os_pool_def = { size, sizeof(T), pool };

public:
   /**
    * Create pool
    */
   Pool() {
      osPoolId pool_id = osPoolCreate(&os_pool_def);
      assert((void*)pool_id == (void*)pool);
   }
   /**
    * Delete pool
    */
   ~Pool() {
   }
   /**
    * Allocate a memory block from the memory pool.
    *
    * @return Address of the allocated memory block or nullptr in case of no memory available.
    */
   T *alloc() {
      return (T*)osPoolAlloc((osPoolId)pool);
   }
   /**
    * Allocate a memory block from the memory pool and initialise to all zeroes.
    *
    * @return Address of the allocated memory block or nullptr in case of no memory available.
    */
   T *calloc() {
      return (T*)osPoolCAlloc((osPoolId)pool);
   }
   /**
    * Return a memory block to a memory pool.
    */
   void free(T *buffer) {
      osStatus status = osPoolFree((osPoolId)pool, buffer);
      assert(status == osOK);
   }
};
template <class T, size_t size> uint32_t Pool<T,size>::pool[] = {0};

/**
 * Wrapper for CMSIS Thread
 */
class Thread {

private:

   const osThreadDef_t thread_def;
   osThreadId thread_id;

public:
   /**
    * Create thread
    *
    * @param argument   Argument passed to thread function
    * @param priority   Priority of thread e.g. osPriorityNormal
    * @param instances  Number of times this thread will be instantiated
    * @param stackSize  Stack size for thread or 0 to indicate default
    */
   Thread(
         void (*threadFunction)(const void *),
         void       *argument=nullptr,
         osPriority  priority=osPriorityNormal,
         uint32_t    instances=1,
         uint32_t    stackSize=0) :
            thread_def {threadFunction, priority, instances, stackSize }
   {
      thread_id = osThreadCreate(&thread_def, argument);
      USBDM::setAndCheckCmsisErrorCode((thread_id != nullptr)?osOK:osErrorOS);
   };
   /**
    * Delete thread
    */
   ~Thread() {
      osThreadTerminate(thread_id);
   }
   /**
    * Get thread ID
    *
    * @return ID of thread
    */
   osThreadId getId() {
      return thread_id;
   }
   /**
    * Get thread ID of current process
    *
    * @return ID of thread
    */
   static osThreadId getMyId() {
      return osThreadGetId();
   }
   /**
    * Get thread Priority
    *
    * @return priority of thread
    */
   osPriority getPriority() {
      return osThreadGetPriority(thread_id);
   }
   /**
    * Set thread Priority
    *
    * @param priority Priority to set for thread
    */
   osStatus setPriority(osPriority priority) {
      return osThreadSetPriority(thread_id, priority);
   }
   /**
    * Set thread Priority
    *
    * @param priority Priority to set for thread
    */
   osStatus yield() {
      return osThreadYield();
   }
   /**
    * Set the specified Signal Flags
    *
    * @param flags    Flags to set
    */
   int32_t signalSet(int32_t flags) {
      return osSignalSet(thread_id, flags);
   }
   /**
    * Clear the specified Signal Flags
    *
    * @param flags    Flags to clear
    */
   int32_t signalClear(int32_t flags) {
      return osSignalClear(thread_id, flags);
   }
   /**
    * Wait for one or more Signal Flags to become signaled for the current thread.
    *
    * @param flags    Flags to wait on
    * @param millisec How long to wait in milliseconds. Use osWaitForever for indefinite wait
    *
    * @return Structure describing event occurring
    */
   static osEvent signalWait(int32_t flags, uint32_t millisec=osWaitForever) {
      return osSignalWait(flags, millisec);
   }

};

/**
 * Wrapper for CMSIS Message Queue
 *
 *
 * @tparam T      Type of items in message queue
 * @tparam size   Size of message queue in items
 *
 * Example:
 * @code
 *  struct Data {
 *     int a;
 *     int b;
 *  };
 *
 *  CMSIS::Pool<Data, 20> *pool = new CMSIS::Pool<Data, 20>;
 *  CMSIS::Message<Data, 10> *messageQ = new  CMSIS::Message<Data, 10>();
 *
 *  for (unsigned i=0; i<5; i++) {
 *     Data *item = pool->alloc();
 *     assert(item != nullptr);
 *     item->a = i;
 *     item->b = i*i;
 *     messageQ->put((uint32_t)item);
 *  }
 *  for (unsigned i=0; ; i++) {
 *     osEvent event = messageQ->get(0);
 *     if (event.status != osEventMessage) {
 *        break;
 *     }
 *     Data *item = (Data*)(event.value.p);
 *     assert(item != nullptr);
 *     pool->free(item);
 *  }
 * @endcode
 */
template <typename T, size_t size>
class Message {

private:
   static uint32_t       queue[4+size];
   const osMessageQDef_t os_pool_def = { size, queue };

//   uint32_t           os_message_cb[2] = { 0 };
//   osSemaphoreDef_t   os_message_def = { os_message_cb };

public:
   /**
    * Create message
    */
   Message(Thread *thread=nullptr) {
      osThreadId threadId = 0;
      if (thread != 0) {
         threadId = thread->getId();
      }
      osMessageQId message_id = osMessageCreate(&os_pool_def, threadId);
      assert((void*)message_id == (void*)queue);
   }
   /**
    * Delete message
    */
   ~Message() {
   }
   /**
    * Put message to queue.
    * Waits indefinitely.
    *
    * @param info     Message to send
    *
    * @return
    * osOK: the message is put into the queue.
    * osErrorParameter: a parameter is invalid or outside of a permitted range.
    */
   void put(T *info) {
      osStatus status = osMessagePut((osMessageQId)queue, (uint32_t)info, osWaitForever);
      assert(status == osOK);
   }
   /**
    * Put message to queue.
    * Returns immediately (for use in ISRs)
    *
    * @param info     Message to send
    *
    * @return
    * osOK: the message is put into the queue.
    * osErrorResource: no memory in the queue was available.
    * osErrorParameter: a parameter is invalid or outside of a permitted range.
    */
   osStatus putISR(uint32_t info) {
      return osMessagePut((osMessageQId)queue, info, 0);
   }
   /**
    * Put message to queue with timeout
    *
    * @param millisec How long to wait in milliseconds. Use osWaitForever for indefinite wait
    *
    * @return
    * osOK: the message is put into the queue.
    * osErrorResource: no memory in the queue was available.
    * osErrorTimeoutResource: no memory in the queue was available during the given time limit.
    * osErrorParameter: a parameter is invalid or outside of a permitted range.
    */
   osStatus put(uint32_t info, uint32_t millisec) {
      return osMessagePut((osMessageQId)queue, info, millisec);
   }
   /**
    * Get message from queue.
    *
    * @param millisec How long to wait in milliseconds. Use osWaitForever for indefinite wait
    *
    * @return
    * Status and Error Codes
    * - osOK: no message is available in the queue and no timeout was specified.
    * - osEventTimeout: no message has arrived during the given timeout period.
    * - osEventMessage: message received, value.p contains the pointer to message.
    * - osErrorParameter: a parameter is invalid or outside of a permitted range.
    */
   osEvent get(uint32_t millisec=osWaitForever) {
      return osMessageGet((osMessageQId)queue, millisec);
   }
   /**
    * Get message from queue.
    * Returns immediately (for use in ISRs)
    *
    * @return
    * Status and Error Codes
    * - osOK: no message is available in the queue.
    * - osEventMessage: message received, value.p contains the pointer to message.
    * - osErrorParameter: a parameter is invalid or outside of a permitted range.
    */
   osEvent getISR() {
      osEvent event = osMessageGet((osMessageQId)queue, 0);
      assert (event.status != osErrorParameter);
      return event;
   }
};
template <class T, size_t size> uint32_t Message<T,size>::queue[] = {0};

}; // end namespace CMSIS

#endif /* PROJECT_HEADERS_CMSIS_H_ */
