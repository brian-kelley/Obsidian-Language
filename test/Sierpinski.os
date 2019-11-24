proc main: void()
{
  uint ctr = 1;
  uint one = 1;
  for i: 0,16
  {
    uint iter = ctr;
    for j: 0,16
    {
      if(iter == 0)
        break;
      if((iter & 1) == 0)
        print(' ');
      else
        print('*');
      iter = iter >> 1;
    }
    print('\n');
    ctr = ctr ^ (ctr << 1);
  }
}
