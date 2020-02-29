proc main: void()
{
  arr: int[][][] = array int[3][3][3];
  for [i, j, k, iter] : arr
  {
    arr[i][j][k] = i * 9 + j * 3 + k;
  }
  for [i, j, k, iter] : arr
  {
    print(iter, '\n');
  }
}
