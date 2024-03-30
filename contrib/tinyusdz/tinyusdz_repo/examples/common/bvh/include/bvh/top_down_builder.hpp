#ifndef BVH_TOP_DOWN_BUILDER_HPP
#define BVH_TOP_DOWN_BUILDER_HPP

#include <stack>
#include <cassert>

namespace bvh {

/// Base class for top-down build tasks.
class TopDownBuildTask {
protected:
    struct WorkItem {
        size_t node_index;
        size_t begin;
        size_t end;
        size_t depth;

        WorkItem() = default;
        WorkItem(size_t node_index, size_t begin, size_t end, size_t depth)
            : node_index(node_index), begin(begin), end(end), depth(depth)
        {}

        size_t work_size() const { return end - begin; }
    };
};

/// Base class for top-down BVH builders.
class TopDownBuilder {
public:
    /// Threshold (number of primitives) under which the builder
    /// doesn't spawn any more OpenMP tasks.
    size_t task_spawn_threshold = 1024;

    /// Maximum depth of the generated tree. This can be used to make
    /// sure the required traversal stack size is under a given constant.
    size_t max_depth = 64;

    /// Largest permissible leaf size. The builder will attempt to split
    /// using a median split on the largest axis as a default strategy
    /// to avoid creating leaves that are larger than this threshold.
    size_t max_leaf_size = 16;

protected:
    ~TopDownBuilder() {}

    template <typename BuildTask, typename... Args>
    void run_task(BuildTask& task, Args&&... args) {
        using WorkItem = typename BuildTask::WorkItemType;
        std::stack<WorkItem> stack;
        stack.emplace(std::forward<Args&&>(args)...);
        while (!stack.empty()) {
            auto work_item = stack.top();
            assert(work_item.depth <= max_depth);
            stack.pop();

            auto more_work = task.build(work_item);
            if (more_work) {
                if (more_work->first.work_size() > more_work->second.work_size())
                    std::swap(more_work->first, more_work->second);

                stack.push(more_work->second);
                auto first_item = more_work->first;
                if (first_item.work_size() > task_spawn_threshold) {
                    BuildTask new_task(task);
                    #pragma omp task firstprivate(new_task, first_item)
                    { run_task(new_task, first_item); }
                } else {
                    stack.push(first_item);
                }
            }
        }
    }
};

} // namespace bvh

#endif
