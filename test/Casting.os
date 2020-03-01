struct Thing1
{
  proc printout: void()
  {
    print("Int vec: (", a, ", ", b, ")\n");
  }
  a: int;
  b: int;
}

struct Thing2
{
  proc printout: void()
  {
    print("Float vec: (", a, ", ", b, ")\n");
  }
  a: float;
  b: float;
}

proc printThing
: void(val: int)
{
  print("Got int: ", val, '\n');
}
: void(val: ulong)
{
  print("Got ulong: ", val, '\n');
}
: void(val: char)
{
  print("Got char: ", val, '\n');
}
: void(val: double)
{
  print("Got double: ", val, '\n');
}
: void(val: float)
{
  print("Got float: ", val, '\n');
}
: void(val: int[])
{
  print("Got int array: ", val, '\n');
}
: void(val: (int, int, int))
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
    someNum: double = 12 * 3.14159265;
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
    raw1: (int, int) = [5, 6];
    print("Raw tuple 1: ", raw1, '\n');
    print("raw 1 as Thing1: ");
    (raw1 as Thing1).printout();
    print("raw 1 as Thing2: ");
    (raw1 as Thing2).printout();
    raw2: (double, int) = [5.3, 6];
    print("Raw tuple 2: ", raw2, '\n');
    print("raw 2 as Thing1: ");
    (raw2 as Thing1).printout();
    print("raw 2 as Thing2: ");
    (raw2 as Thing2).printout();
  }
}

