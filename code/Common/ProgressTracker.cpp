/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team



All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
*/

#include <assimp/ProgressTracker.h>

#include <cassert>

namespace Assimp {

thread_local ProgressTracker *g_progressTracker = nullptr;

ProgressTracker::ProgressTracker() = default;

ProgressTracker::~ProgressTracker() {
    if (g_progressTracker == this) {
        g_progressTracker = nullptr;
    }

    assert(currentScope == nullptr && "ProgressScope still exists during ProgressTracker destruction");
}

void ProgressTracker::SetThreadLocalProgressTracker(ProgressTracker *tracker) {
    g_progressTracker = tracker;
}

ProgressScope::ProgressScope(const char *scopeName) :
        scopeName(scopeName) {

    progressTracker = g_progressTracker;

    if (progressTracker) {
        progressTracker->Lock();
        parentScope = progressTracker->currentScope;
        progressTracker->currentScope = this;
        progressTracker->Unlock();
    }

    if (parentScope) {
        indentation = parentScope->indentation;
    }

    // this is used to propagate the scope name right away
    // so that the newly started operation name shows up
    SetCompletion(0.0f, "Begin");

    ++indentation;
}

ProgressScope::~ProgressScope() {

    --indentation;

    if (progressTracker == nullptr)
        return;

    progressTracker->Lock();

    SetCompletion(1.0f, "End");

    progressTracker->currentScope = parentScope;
    progressTracker->Unlock();

    progressTracker = nullptr;
}

void ProgressScope::SetCompletion(float fraction, const char *displayText /*= nullptr*/) {

    if (progressTracker)
        progressTracker->Lock();

    assert(fraction >= currentCompletion && "Completion progress should always move forwards");
    assert(fraction >= 0.0f && fraction <= 1.0f && "Completion progress should be between 0 and 1");
    currentCompletion = fraction;

    if (displayText == nullptr)
        displayText = "";

    if (parentScope) {
        parentScope->PropagateChildCompletion(currentCompletion, scopeName, indentation, displayText);
    } else if (progressTracker) {
        progressTracker->ProgressUpdate(currentCompletion, scopeName, indentation, displayText);
    }

    if (progressTracker)
        progressTracker->Unlock();
}

void ProgressScope::AddSteps(size_t numSteps) {

    assert(activeStep == -1 && "Steps have to be added at the very beginning");

    const size_t nOld = stepWeights.size();
    const size_t nNew = nOld + numSteps;
    stepWeights.resize(nNew);

    for (size_t i = nOld; i < nNew; ++i) {
        stepWeights[i] = 1.0f;
    }

    totalExpectedWeight += numSteps;
}

void ProgressScope::AddStep(float weight /*= 1.0f*/) {

    assert(activeStep == -1 && "Steps have to be added at the very beginning");

    stepWeights.push_back(weight);
    totalExpectedWeight += weight;
}

void ProgressScope::StartStep(const char *displayText /*= nullptr*/) {

    if (progressTracker)
        progressTracker->Lock();

    if (activeStep == -1) {
        SetCompletion(0.0f, displayText);
    }

    if (activeStep >= 0 && activeStep < stepWeights.size()) {

        float addCompletion = stepWeights[activeStep] / totalExpectedWeight;
        SetCompletion(std::min(currentCompletion + addCompletion, 1.0f), displayText);

        baseCompletion = currentCompletion;
    }

    ++activeStep;
    assert(activeStep < stepWeights.size() && "Attempting to start more steps than were added");

    if (progressTracker)
        progressTracker->Unlock();
}

void ProgressScope::PropagateChildCompletion(float childCompletion, const char *currentScope, int indent, const char *displayText) {

    assert(activeStep < stepWeights.size() && "Not enough steps added to ProgressScope for the number of child scopes used.");

    float curStepWeight = 1.0f;

    if (activeStep >= stepWeights.size())
        return;

    if (activeStep >= 0) {
        curStepWeight = stepWeights[activeStep];
    }

    const float stepCompletion = childCompletion * curStepWeight / totalExpectedWeight;
    const float totalCompletion = baseCompletion + stepCompletion;

    if (parentScope) {
        parentScope->PropagateChildCompletion(totalCompletion, currentScope, indent, displayText);
    } else if (progressTracker) {
        progressTracker->ProgressUpdate(totalCompletion, currentScope, indent, displayText);
    }
}

} // Namespace Assimp
