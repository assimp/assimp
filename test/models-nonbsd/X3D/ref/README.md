# X3D 3D model reference images

## ChevyTahoe.x3d

### Working (using IRR xml parser)
ChevyTahoe as of 29 Sep 2020 git commit `6a4c338`, using IRR xml parsing, renders OK:

<img alt="ChevyTahoe.x3d (irr_xml)" src="screenshots%2FChevyTahoe_x3d_irr_xml.png" width=320 />

### Broken (using pugi xml parser)
ChevyTahoe as of 1 Dec 2021 git commit `1614934`, using pugi xml parsing, renders with 
missing meshes and obvious artifacts:

<img alt="ChevyTahoe.x3d (pugi_xml)" src="screenshots%2FChevyTahoe_x3d_pugi_xml.png" width=320 />
