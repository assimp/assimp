/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team


All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

/** @file ProgressTracker.h
 */

#pragma once
#ifndef AI_PROGRESSTRACKER_H
#define AI_PROGRESSTRACKER_H

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/types.h>

#include <vector>

namespace Assimp {

// -------------------------------------------------------------------------------
/**
 * Abstract base class for receiving progress information.
 *
 * Derive and implement the ProgressUpdate() function.
 * To receive progress updates call ProgressTracker::SetThreadLocalProgressTracker() with your instance.
 *
 * On the running thread, this instance will then be used to report back how much of the process has completed and
 * which operation is currently running.
 *
 * Note: Whether any progress gets reported and how detailed the reporting is, depends on how well the executing
 * code paths are peppered with instances of ProgressScope (see below).
 *
 * You can also use ProgressScope in your own code on top of assimp, such that assimp operations become only
 * one part of the overall progress.
 */
// -------------------------------------------------------------------------------
class ASSIMP_API ProgressTracker {

public:
    ProgressTracker();
    virtual ~ProgressTracker();

    /** Makes the given instance the currently active one on this thread.
     *
     * This allows to load models on multiple threads and separate their progress reporting.
     * */
    static void SetThreadLocalProgressTracker(ProgressTracker *tracker);

    /** In case the derived class needs to access shared resources, this can be used to lock a mutex. */
    virtual void Lock(){};

    /** In case the derived class needs to access shared resources, this can be used to unlock a mutex. */
    virtual void Unlock(){};

    /** Called whenever there is a change to the current progress to report
     *
     *  @param totalCompletion Value between 0 and 1 that represents how much of all (known) work has been finished.
     *  @param currentScopeName The name of the ProgressScope that's currently active.
     *  @param scopeLevel How deep the nesting of ProgressScope's currently is. Can be used for indenting log output.
     *  @param displayText The text that was passed to ProgressScope::SetCompletion() or ProgressScope::StartStep()
     *         to show to users, that describes what operation is currently being done.
     * */
    virtual void ProgressUpdate(float totalCompletion, const char *currentScopeName, int scopeLevel, const char *displayText) = 0;

private:
    friend class ProgressScope;
    ProgressScope *currentScope = nullptr;
};

// -------------------------------------------------------------------------------
/**
 * Instantiate this class locally inside functions to report progress back to the user.
 *
 * Suppose you have a function "LoadX" that does three things:
 * 1) Read a file from disk into memory.
 * 2) Tokenize the data.
 * 3) Convert the data to an aiScene.
 *
 * To report progress back, instantiate a ProgressScope at the top of the function,
 * then call AddStep() three times.
 * If the steps are known to take very different amounts of time, you can give each step a weight.
 * For example if 1) takes 20% of the time, 2) takes 10% and 3) takes 70%, you can use the step weights 20, 10, 70
 * or 0.2, 0.1, 0.7. The weights get normalized, so use whatever is more convenient.
 *
 * Every time a new 'phase' starts, call StartStep(). This computes the total completion and reports it back
 * to the user through ProgressTracker.
 *
 * Within a step you can use nested ProgressScope's to make progress reporting more fine granular.
 * For instance within reading the file one could use a nested scope to report back how many bytes have been read.
 * 
 * In some cases it is easier to just specify the progress directly, rather than using steps.
 * Call SetCompletion() in such situations. For example when reading a file, you should not report progress for
 * every byte read, but you can read an entire chunk of data and then report the percentage.
 */
// -------------------------------------------------------------------------------
class ASSIMP_API ProgressScope final {

public:
    ProgressScope(const char *scopeName);
    ~ProgressScope();

    /** Specifies the 0-1 value of progress for this scope directly.
      *
      * When using this function, you shouldn't also use steps.
      *
      * This function reports the local progress up the chain of parent scopes and combines all their step weights
      * to ultimately report a single total completion value to ProgressTracker.
    . */
    void SetCompletion(float fraction, const char *displayText = nullptr);

    /** Adds a number of steps that are expected to be executed in this scope.
     *
     * Each step has equal weighting, meaning it is expected that every step takes approximately the same amount of time.
     * */
    void AddSteps(size_t numSteps);

    /** Adds a single step that is expected to be executed in this scope.
     *
     * The step can optionally be weighted (all weights are normalized later).
     * This is used to indicate that a step takes relatively less or more time than other steps within the same scope.
     * */
    void AddStep(float weight = 1.0f);

    /** Reports to the ProgressTracker that new progress has been made.
     *
     * Set the display string to also indicate what work is currently being done.
     *
     * This will compute the overall progress for this scope and call SetCompletion() internally.
     * */
    void StartStep(const char *displayText = nullptr);

private:
    void PropagateChildCompletion(float childCompletion, const char *currentScope, int indent, const char *displayText);

    ProgressScope *parentScope = nullptr;
    ProgressTracker *progressTracker = nullptr;
    const char *scopeName = nullptr;
    int activeStep = -1;
    std::vector<float> stepWeights;
    float totalExpectedWeight = 0.0f;
    float currentCompletion = 0.0f;
    float baseCompletion = 0.0f;
    int indentation = 0;
};

} // Namespace Assimp

#endif // AI_PROGRESSTRACKER_H
