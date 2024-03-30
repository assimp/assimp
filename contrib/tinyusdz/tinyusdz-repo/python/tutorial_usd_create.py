import tinyusdz


root = tinyusdz.Model(name="model")
xform = tinyusdz.Xform(name="xform1")

print(root.primChildren())
# for child in root.primChildren()
# Print Prim.
print(root)


# Dump USD to string(USDA)
print(tinyusdz.dumps(root))

# Save USD as USDA(Ascii format only)
tinyusdz.save(root, "output.usda")
