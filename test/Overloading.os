func getSize
: int(int i)
{
  return 4;
}
: int(double d)
{
  return 8;
}
: int(char c)
{
  return 1;
}
: int(string s)
{
  return s.len;
}

func twice
: char(char c)
{
  return c * 2;
}
: int(int i)
{
  return i * 2;
}
: double(double d)
{
  return d * 2;
}
: string(string s)
{
  return s + s;
}

proc main: void()
{
  print("Size of int: ", getSize(3), '\n');
  print("Size of char: ", getSize('a'), '\n');
  print("Size of string hello: ", getSize("hello"), '\n');
  print("Size of double: ", getSize(3.14159), '\n');
  print("Twice '!': ", twice('!'), '\n');
  print("Twice -4: ", twice(-4), '\n');
  print("Twice 2.718: ", twice(2.718), '\n');
  print("Twice hello: ", twice("hello"), '\n');
}

