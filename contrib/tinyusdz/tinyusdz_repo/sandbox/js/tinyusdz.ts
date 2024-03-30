// Prim path
class Path {
  prim_part: string;
  prop_part: string;
 
}

class Prim {
  element_name: string;
  id: number; // 0 or null = Invalid

}

class Stage {
  name: string;
  id: number;

  primChildren: Array<Prim>;
}
