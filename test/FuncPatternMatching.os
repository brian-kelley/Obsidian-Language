//Example procedure that either prints a string,
//or prints an error message saying there is no string

//First, implement a version with match
proc withMatch: void(s: string?)
{
  match(n : s)
  {
    case string:
    {
      print(n, '\n');
    }
    case error:
    {
      print("Error\n");
    }
  }
}

proc withIs: void(string? s)
{
  if(s is error)
    print("Error\n");
  else
    print(s as string, '\n');
}

proc withPattern
: void(string s)
{
  print(s, '\n');
}
: void(error e)
{
  print("Error\n");
}

proc main: void()
{
  aString: string? = "hello";
  anError: string? = error;
  withMatch(aString);
  withMatch(anError);
  withIs(aString);
  withIs(anError);
  withPattern(aString);
  withPattern(anError);
}

