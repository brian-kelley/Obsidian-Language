func bubbleSort: int[] (arr: int[])
{
  update: bool = true;
  while(update)
  {
    update = false;
    for i: 0, arr.len - 1
    {
      if(arr[i] > arr[i + 1])
      {
        update = true;
        temp: int = arr[i];
        arr[i] = arr[i + 1];
        arr[i + 1] = temp;
      }
    }
  }
  return arr;
}

proc insertionSort: int[] (arr: int[])
{
  for i: 1, arr.len
  {
    j: int = i - 1;
    moving: int = arr[i];
    //this tests short-circuit evaluation
    while(j >= 0 && arr[j] > moving)
    {
      arr[j + 1] = arr[j];
      j--;
    }
    arr[j + 1] = moving;
  }
  return arr;
}

func selectionSort: int[] (arr: int[])
{
  for i: 0, arr.len - 1
  {
    minIndex: int = i;
    minVal: int = arr[i];
    for j: i + 1, arr.len
    {
      if(arr[j] < minVal)
      {
        minIndex = j;
        minVal = arr[j];
      }
    }
    if(minIndex != i)
    {
      temp: int = arr[i];
      arr[i] = arr[minIndex];
      arr[minIndex] = temp;
    }
  }
  return arr;
}

proc main: void()
{
  testArr: int[] = [67, -234, 63, 786, 13412, -234, 754324];
  print("Bubble sorted:    ", bubbleSort(testArr), '\n');
  print("Insertion sorted: ", insertionSort(testArr), '\n');
  print("Selection sorted: ", selectionSort(testArr), '\n');
  //make sure the original isn't modified after being passed around
  print("Original:         ", testArr, '\n');
}

