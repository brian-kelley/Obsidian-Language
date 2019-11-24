struct A
{
  proc doThing: void()
  {
    print("A is doing a thing.\n");
  }
  proc doAThing: void()
  {
    print("Specific A thing, my value is ", dataA, '\n');
  }
  func doubleMe: int()
  {
    return dataA * 2;
  }
  static proc staticAThing: void()
  {
    print("A string: ", label, '\n');
    label = "Already printed it.";
  }
  int dataA;
  static string label;
}

struct B
{
  proc doThing: void()
  {
    print("B is doing a thing.\n");
  }
  proc doBThing: void()
  {
    print("Specific B thing, my value is ", dataB, '\n');
  }
  func int getSum()
  {
    return a.dataA + dataB;
  }
  ^A a;
  int dataB;
}

struct C
{
  proc doThing: void()
  {
    print("C is doing a thing.\n");
  }
  proc doCThing: void()
  {
    print("Specific C thing, my value is ", dataC, '\n');
  }
  func getProduct: int()
  {
    return b.a.dataA * b.dataB * dataC;
  }
  ^B b;
  int dataC;
}

proc main: void()
{
  A a = [4];
  B b = [[5], 6];
  C c = [[[1], 2], 8];
  print("Exercising A.\n");
  a.doThing();
  a.doAThing();
  print("Doubled: ", a.doubleMe(), '\n');
  print("Exercising B.\n");
  b.doThing();
  b.doAThing();
  b.doBThing();
  print("Summed: ", b.getSum(), '\n');
  print("Exercising C.\n");
  c.doThing();
  c.doAThing();
  c.doBThing();
  c.doCThing();
  print("Multiplied: ", c.getProduct(), '\n');
  print("Testing A static:\n");
  A.staticAThing();
  A.staticAThing();
  A.label = "Third time";
  A.staticAThing();
}

