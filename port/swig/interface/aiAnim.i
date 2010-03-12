%{
#include "aiAnim.h"
%}

ASSIMP_ARRAY(aiAnimation, aiNodeAnim*, mChannels, $self->mNumChannels);

%include "aiAnim.h"
