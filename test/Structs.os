struct A
{
  proc void doThing()
  {
    print("A is doing a thing.\n");
  }
  proc void doAThing()
  {
    print("Specific A thing, my value is ", dataA, '\n');
  }
  func int doubleMe()
  {
    return dataA * 2;
  }
  static proc void staticAThing()
  {
    print("A string: ", label, '\n');
    label = "Already printed it.";
  }
  int dataA;
  static string label;
}

struct B
{
  proc void doThing()
  {
    print("B is doing a thing.\n");
  }
  proc void doBThing()
  {
    print("Specific B thing, my value is ", dataB, '\n');
  }
  func bool getSum()
  {
    return a.dataA + dataB;
  }
  ^A a;
  int dataB;
}

struct C
{
  proc void doThing()
  {
    print("C is doing a thing.\n");
  }
  proc void doCThing()
  {
    print("Specific C thing, my value is ", dataC, '\n');
  }
  func int getProduct()
  {
    return b.a.dataA * b.dataB * dataC;
  }
  ^B b;
  int dataC;
}

proc void main()
{
  A a = [4];
  B b = [6];
  C c = [8];
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
