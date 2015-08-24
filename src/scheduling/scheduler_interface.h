// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// The scheduler interface assumed by the engine.

#ifndef FIRMAMENT_SCHEDULING_SCHEDULER_INTERFACE_H
#define FIRMAMENT_SCHEDULING_SCHEDULER_INTERFACE_H

#include <set>

#include "base/common.h"
#include "messages/base_message.pb.h"
#include "base/job_desc.pb.h"
#include "base/types.h"
#include "base/task_final_report.pb.h"
#include "misc/printable_interface.h"
#include "engine/executor_interface.h"
#include "engine/topology_manager.h"
#include "storage/object_store_interface.h"

namespace firmament {
namespace scheduler {

using machine::topology::TopologyManager;
using store::DataObjectMap_t;
using store::ObjectStoreInterface;

class SchedulerInterface : public PrintableInterface {
 public:
  SchedulerInterface(shared_ptr<JobMap_t> job_map,
                     shared_ptr<ResourceMap_t> resource_map,
                     ResourceTopologyNodeDescriptor* resource_topology,
                     shared_ptr<ObjectStoreInterface> object_store,
                     shared_ptr<TaskMap_t> task_map)
      : job_map_(job_map),  resource_map_(resource_map), task_map_(task_map),
        object_store_(object_store), resource_topology_(resource_topology) {}

  // Adds a job to the set of active jobs that are considered for scheduling.
  // TODO(malte): Determine if we actually need this, given the reactive design
  // of the scheduler.
  //void AddJob(shared_ptr<JobDescriptor> job_desc);

  /**
   * Finds the resource to which a particular task ID is currently bound.
   * @param task_id the id of the task fow which to do the lookup
   * @return NULL if the task does not exist or is not currently bound.
   * Otherwise, it returns its resource id
   */
  virtual ResourceID_t* BoundResourceForTask(TaskID_t task_id) = 0;

  /**
   * Checks if all running tasks managed by this scheduler are healthy. It
   * invokes failure handlers if any failures are detected.
   */
  virtual void CheckRunningTasksHealth() = 0;

  /**
   * Unregisters a resource ID from the scheduler. No-op if the resource ID is
   * not actually registered with it.
   * @param res_id the id of the resource to de-register
   */
  virtual void DeregisterResource(ResourceID_t res_id) = 0;

  virtual executor::ExecutorInterface *GetExecutorForTask(TaskID_t task_id) = 0;

  // TODO(malte): comment
  virtual void HandleReferenceStateChange(const ReferenceInterface& old_ref,
                                          const ReferenceInterface& new_ref,
                                          TaskDescriptor* td_ptr) = 0;

  /**
   * Handles the completion of a job (all tasks are completed, failed or
   * aborted). May clean up scheduler-specific state.
   * @param job_id the id of the completed job
   */
  virtual void HandleJobCompletion(JobID_t job_id) = 0;

  /**
   * Handles the completion of a task. This usually involves freeing up its
   * resource by setting it idle, and recording any bookkeeping data required.
   * @param td_ptr the task descriptor of the completed task
   * @param report the task report to be populated with statistics
   * (e.g., finish time).
   */
  virtual void HandleTaskCompletion(TaskDescriptor* td_ptr,
                                    TaskFinalReport* report) = 0;
  /**
   * Handles the failure of an attempt to delegate a task to a subordinate
   * coordinator. This can happen because the resource is no longer there (it
   * failed) or it is no longer idle (someone else put a task there).
   * @param td_ptr the descriptor of the task that could not be delegated
   */
  virtual void HandleTaskDelegationFailure(TaskDescriptor* td_ptr) = 0;

  /**
   * Handles the failure of a task. This usually involves freeing up its
   * resource by setting it idle, and kicking off the necessary fault tolerance
   * handling procedures.
   * @param td_ptr the task descriptor of the failed task
   */
  virtual void HandleTaskFailure(TaskDescriptor* td_ptr) = 0;

  /**
   * Places a task delegated from a superior coordinator to a resource managed
   * by this scheduler.
   * @param td_ptr the task descriptor of the delegated task
   * @param targer_resource the resource on which to place the task
   */
  virtual bool PlaceDelegatedTask(TaskDescriptor* td_ptr,
                                  ResourceID_t target_resource) = 0;

  /**
   * Kills a running task.
   * @param task_id the id of the task to kill
   * @param reason the reason to kill the task
   */
  virtual void KillRunningTask(TaskID_t task_id,
                               TaskKillMessage::TaskKillReason reason) = 0;

  /**
   * Registers a resource ID with the scheduler, who may subsequently assign
   * work to this resource.
   * @param res_id the id of the resource
   * @param local boolean to indicate if the resource is local or not
   */
  // TODO(malte): Add support for registering a resource with multiple
  // schedulers.
  virtual void RegisterResource(ResourceID_t res_id, bool local) = 0;

  /**
   * Finds runnable tasks for the job in the argument and adds them to the
   * global runnable set.
   * @param jd_ptr the descriptor of the job for which to find tasks
   */
  virtual const set<TaskID_t>& RunnableTasksForJob(JobDescriptor* jd_ptr) = 0;

  /**
   * Schedules all runnable tasks in a job.
   * @param jd_ptr the job descriptor for which to schedule tasks
   * @return the number of tasks scheduled.
   */
  virtual uint64_t ScheduleJob(JobDescriptor* jd_ptr) = 0;

  // Runs a scheduling iteration for all active jobs.
  // TODO(malte): Determine if the need this, given the reactive design of the
  // scheduler.
  //void ScheduleAllJobs();

 protected:
  /**
   * Binds a task to a resource, i.e. effects a scheduling assignment. This will
   * modify various bits of meta-data tracking assignments. It will then
   * delegate the actual execution of the task binary to the appropriate local
   * execution handler.
   * @param td_ptr the descriptor of the task to bind
   * @param rd_ptr the descriptor of the resource to bind to
   */
  virtual void BindTaskToResource(TaskDescriptor* td_ptr,
                                  ResourceDescriptor* rd_ptr) = 0;

  /**
   * Finds a resource for a runnable task. This is the core placement logic.
   * @param td_ptr the descriptor of the task for which to find resources
   * @return the resource ID of the resource chosen in the second argument, or
   * NULL if no resource could be found.
   */
  virtual const ResourceID_t* FindResourceForTask(TaskDescriptor* td_ptr) = 0;

  // Pointers to the associated coordinator's job, resource and object maps
  shared_ptr<JobMap_t> job_map_;
  shared_ptr<ResourceMap_t> resource_map_;
  shared_ptr<TaskMap_t> task_map_;
  shared_ptr<store::ObjectStoreInterface> object_store_;
  // Resource topology (including any registered remote resources)
  ResourceTopologyNodeDescriptor* resource_topology_;
};

}  // namespace scheduler
}  // namespace firmament

#endif  // FIRMAMENT_SCHEDULING_SCHEDULER_INTERFACE_H
