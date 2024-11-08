# meshlab code provenance

Parser/Scanner copied verbatim from
  [meshlab io_x3d/vrml](https://github.com/cnr-isti-vclab/meshlab/tree/main/src/meshlabplugins/io_x3d/vrml),
as of [19 Sep 2024 commit 8fd4c52](https://github.com/cnr-isti-vclab/meshlab/commit/8fd4c52bc43872e34a1ec2c4a0e60aaf1730e918)
then modified to use pugixml

## pugixml notes
Note that it isn't possible to add an unattached pugixml Node object, modify it, then
add it to the tree later; the node needs to be attached somewhere in the tree on instantiation
(even if it's a temporary at root level to be removed when finished) before populating, adding
children etc.
