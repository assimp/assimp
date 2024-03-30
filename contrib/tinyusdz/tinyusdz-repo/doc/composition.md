# subLayers

import layer(USD)

# inherits

Inherit Prim in the layer

# variantSets

Works like `switch`.

# references

load layer(USD) from file(asset)

# payloads

delayed version of `references`

# specializes

Something like C++ template specialization.
Override existing Prim
(Note: `over` Prim spec cannot be used in same Layer(USD file), but can do it with specializes)

## References

https://github.com/ColinKennedy/USD-Cookbook/blob/master/concepts/asset_composition_arcs.md


## LIVRPS

* Local: Load subLayer
* Inherit: Resolve inherits
  * Recursively apply LIVRP to target path(no Specializes evaluation)
* Variants: Resolve VariantSets with currently selected Variants
  * Recursively apply LIVRP to target layer(no Specializes evaulation)
* References: Load references USD
  * Recursively aply LIVRP to target layer(no Specializes evaulation)
* Payload: Load payload USD
  * Recursively aply LIVRP to target layer(no Specializes evaulation)
* Specializes: Resolve specializes
  * Recursively aply LIVRPS to target layer(do Specializes evaulation)

  


