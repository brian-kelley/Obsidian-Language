proc main: void()
{
  arr: string[] = ["Hello", "World", "ABC"];
  for [i, iter] : arr
  {
    print(i, ": ", iter, '\n');
  }
}
