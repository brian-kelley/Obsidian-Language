struct Thing1
{
  proc printout: void()
  {
    print("Int vec: (", a, ", ", b, ")\n");
  }
  int a;
  int b;
}

struct Thing2
{
  proc printout: void()
  {
    print("Float vec: (", a, ", ", b, ")\n");
  }
  float a;
  float b;
}

proc printThing
: void(int val)
{
  print("Got int: ", val, '\n');
}
: void(ulong val)
{
  print("Got ulong: ", val, '\n');
}
: void(char val)
{
  print("Got char: ", val, '\n');
}
: void(double val)
{
  print("Got double: ", val, '\n');
}
: void(float val)
{
  print("Got float: ", val, '\n');
}
: void(int[] val)
{
  print("Got int array: ", val, '\n');
}
: void((int, int, int) val)
{
  print("Got 3-int tuple: ", val, '\n');
}

proc main: void()
{
  //Convert int to different types
  {
    printThing(35);
    printThing(35 as ulong);
    printThing(35 as char);
    printThing(35 as double);
    printThing(35 as float);
  }
  //Convert/truncate double to different types
  {
    double someNum = 12 * 3.14159265;
    printThing(someNum as int);
    printThing(someNum as ulong);
    printThing(someNum as char);
    printThing(someNum as double);
    printThing(someNum as float);
    printThing([someNum] as int[]);
    //first match for this should be int[]
    printThing([someNum, someNum, someNum]);
    //explicitly a tuple
    printThing([someNum, someNum, someNum] as (int, int, int));
  }
  //Try array <-> tuple conversion
  {
    printThing([35]);                  //1-elem array
    printThing([35, 34, 33]);          //3-int tuple
    printThing([35, 34, 33] as int[]); //3-elem array
    printThing([35, 34, 33, 32]);      //4-elem array
  }
  //Try casting to struct and calling
  {
    (int, int) raw1 = [5, 6];
    print("Raw tuple 1: ", raw1, '\n');
    print("raw 1 as Thing1: ");
    (raw1 as Thing1).printout();
    print("raw 1 as Thing2: ");
    (raw1 as Thing2).printout();
    (double, int) raw2 = [5.3, 6];
    print("Raw tuple 2: ", raw2, '\n');
    print("raw 2 as Thing1: ");
    (raw2 as Thing1).printout();
    print("raw 2 as Thing2: ");
    (raw2 as Thing2).printout();
  }
}

