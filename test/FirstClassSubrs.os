func A: int(i: int)
{
  return i * 2;
}

func B: int(i: int)
{
  return -i;
}

typedef functype int(int) IntMap;

func apply: int[](input: int[], f: IntMap)
{
  for i : 0, input.len
    input[i] = f(input[i]);
  return input;
}

proc cb1: void()
{
  print("Hello from callback 1!\n");
}

proc cb2: void()
{
  print("Hello from callback 2!\n");
}

typedef proctype void() Callback;

proc main: void()
{
  print("A(5): ", A(5), '\n');
  print("B(5): ", B(5), '\n');
  map1: IntMap = A;
  map2: IntMap = B;
  print("map1(5): ", map1(5), '\n');
  print("map2(5): ", map2(5), '\n');
  arr: int[] = [1, 2, 3, 4, 5];
  print("Array: ", arr, '\n');
  print("Applying A to arr: ", apply(arr, A), '\n');
  print("Applying B to arr: ", apply(arr, B), '\n');
  print("Applying map1 to arr: ", apply(arr, map1), '\n');
  print("Applying map2 to arr: ", apply(arr, map2), '\n');
  events: Callback[] = [cb2, cb1, cb2];
  print("Calling callbacks for all events, in order:\n");
  for [i, e] : events
  {
    e();
  }
}

