func getSize
: int(i: int)
{
  return 4;
}
: int(d: double)
{
  return 8;
}
: int(c: char)
{
  return 1;
}
: int(s: string)
{
  return s.len;
}

func twice
: char(c: char)
{
  return c * 2;
}
: int(i: int)
{
  return i * 2;
}
: double(d: double)
{
  return d * 2;
}
: string(s: string)
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

