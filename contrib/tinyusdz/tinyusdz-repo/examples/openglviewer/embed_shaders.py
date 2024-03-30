# `xxd` command equivalent in python

import os
import sys
import glob

shader_dir = "shaders"
shader_files = glob.glob(os.path.join(shader_dir, "*.frag"))
shader_files += glob.glob(os.path.join(shader_dir, "*.vert"))

for shader_file in shader_files:

    c_filename = os.path.basename(shader_file)
    c_filename = c_filename.replace('.', '_')

    s = ""
    with open(shader_file, 'r') as f:
        buf = f.read()
        buf_x16 = [('0x%02x' % ord(i)) for i in buf]

        # print each 12 items
        buf_chunks = [buf_x16[i:i+12] for i in range(0, len(buf_x16), 12)]

        s += "// input filename: {}\n".format(shader_file)
        s += "unsigned char {}_{}[] = ".format(shader_dir, c_filename) + "{\n"
        for chunk in buf_chunks:
            s += '  '
            s += ', '.join(chunk)
            s += ',\n' # TODO: Do not emit ',' in the last item
        s += "};\n";
        s += "unsigned int {}_{}_len = {};\n".format(shader_dir, c_filename, len(buf));


    out_fname = shader_file + "_inc.hh"
    with open(out_fname, 'w') as wf:
        wf.write(s)

    print("{} => {}".format(shader_file, out_fname))
