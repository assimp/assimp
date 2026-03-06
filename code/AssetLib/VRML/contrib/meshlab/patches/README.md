# meshlab patch

## Notes
Depending on user's OS, build environment etc it may be necessary to change line endings of
`patches/meshlab.patch` file from `CRLF` to `LF` in order for patch operation to succeed

## Overview
"Parser" based on QtXml, need to change to use pugixml

## pugixml notes
Note that it isn't possible to add an unattached pugixml Node object, modify it, then
add it to the tree later; the node needs to be attached somewhere in the tree on instantiation
(even if it's a temporary at root level to be removed when finished) before populating, adding
children etc.
