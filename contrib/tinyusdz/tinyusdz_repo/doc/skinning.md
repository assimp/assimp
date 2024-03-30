# Skinnig in usdSkel

https://graphics.pixar.com/usd/dev/api/_usd_skel__intro.html

## Evaluation of skinning

Terminology

* bindTransforms : World matrices of each joint
* restTransforms : Local matrices of each joint. Need to evaluate in bone(joint) hierarchy. Used when there is no corresponding transform from `SkelAnimation` 
* SkelAnimation : (local?) matrices for each joint
* Sparse : When SkelAnimation supplies(maps?) animation transform to subset of joints
* Non-Sparse : When SkelAnimation supplies(maps?) animation transform to all of joints

`restTransforms` could be optional when `SkelAnimation` supply transforms  for all joints, but it seems usdview and Houdini does not allow omitting `restTransforms`(`restTransforms` is not automatically calculated from `bindTransforms`)

Also, without `bindTransforms` skinning does not work well(it is not automatically caculuated from `restTransforms`) 

So, both `bindTransforms` and `restTransforms` must exist in USD.

### Skinning matrix 


```
skinM = inv(jointWorldSpaceBindTransform) x jointSkelSpaceTransform

jointSkelSpaceTransform = jointLocalSpaceTransform * parentJointSkelSpaceTransform

//

jointWorldSpaceTransform = jointLocalSpaceTransform *
    parentJointSkelSpaceTransform * skelLocalToWorldTransform

```

### Skinning a point

```
# geomBindTransform = `primvars:skel:geomBindTransform` 

skelSpacePoint = geomBindTransform.Transform(localSpacePoint)
p = (0,0,0)
for jointIndex,jointWeight in jointInfluencesForPoint:
    p += skinningTransforms[jointIndex].Transform(skelSpacePoint)*jointWeight
```


EoL.
