struct A
{
  typedef proctype string(int) MessageGetter;

  proc isNegative: string()
  {
    if(value < 0)
      return "Negative";
    else
      return "Nonnegative";
  }

  proc isZero: string()
  {
    if(value == 0)
      return "Zero";
    else
      return "Nonzero";
  }

  getStr: MessageGetter = isZero;
  value: int;
}

proc main: void()
{
  a1: A = [A.isZero, 5];
  a2: A = [A.isNegative, -3];
  a3: A = [A.isZero, 0];
  print(a1.getStr(), '\n');
  print(a2.getStr(), '\n');
  print(a3.getStr(), '\n');
  a2.getStr = A.isNegative;
  a3.getStr = A.isNegative;
  print(a1.getStr(), '\n');
  print(a2.getStr(), '\n');
  print(a3.getStr(), '\n');
}

