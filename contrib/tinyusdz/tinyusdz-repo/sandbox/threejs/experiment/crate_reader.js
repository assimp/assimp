const fs = require('fs')
const lz4js = require('lz4js')

buffer = new ArrayBuffer(12);
let dataView = new DataView(buffer);

dataView.setUint8(1,255)
console.log(dataView.getUint8(1))

const buf = fs.readFileSync("test.usdc")

// magic header(8 bytes)
const h0 = buf.readUInt8(0);
const h1 = buf.readUInt8(1);
const h2 = buf.readUInt8(2);
const h3 = buf.readUInt8(3);
const h4 = buf.readUInt8(4);
const h5 = buf.readUInt8(5);
const h6 = buf.readUInt8(6);
const h7 = buf.readUInt8(7);

console.log(h0)
console.log(h1)
console.log(h2)
console.log('')
console.log('P'.charCodeAt(0))
if ((h0 == 'P'.charCodeAt(0)) && (h1 == 'X'.charCodeAt(0)) && (h2 == 'R'.charCodeAt(0)) && (h3 == '-'.charCodeAt(0)) &&
    (h4 == 'U'.charCodeAt(0)) && (h5 == 'S'.charCodeAt(0)) && (h6 == 'D'.charCodeAt(0)) && (h7 == 'C'.charCodeAt(0))) {
  // ok
} else {
  console.log('invalid magic header')
}

