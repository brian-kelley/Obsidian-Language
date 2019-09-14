proc void main()
{
  string[] arr = ["Hello", "World", "ABC"];
  for [i, iter] : arr
  {
    print(i, ": ", iter, '\n');
  }
}
