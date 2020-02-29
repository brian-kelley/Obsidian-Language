proc main: void()
{
  for i : 2,20
  {
    isPrime: bool = true;
    if(i != 2 && i % 2 == 0)
      isPrime = false;
    else
    {
      for(j: int = 3; j < i; j += 2)
      {
        if(i % j == 0)
        {
          isPrime = false;
          break;
        }
      }
    }
    if(isPrime)
    {
      print(i, '\n');
    }
  }
}
