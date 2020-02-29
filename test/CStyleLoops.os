proc main: void()
{
  {
    //init, cond, incr
    pyramid: int = 0;
    for(i: int = 1; i <= 5; i++)
    {
      pyramid += i * i;
    }
    print(pyramid, '\n');
  }
  {
    //cond, incr
    pyramid: int= 0;
    i: int = 1;
    for(; i <= 5; i++)
    {
      pyramid += i * i;
    }
    print(pyramid, '\n');
  }
  {
    //init, incr
    pyramid: int = 0;
    for(i: int = 1;; i++)
    {
      pyramid += i * i;
      if(i == 5)
        break;
    }
    print(pyramid, '\n');
  }
  {
    //init, cond
    pyramid: int = 0;
    for(i: int = 1; i <= 5;)
    {
      pyramid += i * i;
      i++;
    }
    print(pyramid, '\n');
  }
  {
    //init
    pyramid: int = 0;
    for(i: int = 1;;)
    {
      pyramid += i * i;
      if(i == 5)
        break;
      i++;
    }
    print(pyramid, '\n');
  }
  {
    //cond
    pyramid: int = 0;
    i: int = 1;
    for(; i <= 5;)
    {
      pyramid += i * i;
      if(i == 5)
        break;
      i++;
    }
    print(pyramid, '\n');
  }
  {
    //incr
    pyramid: int = 0;
    i: int = 1;
    for(;; i++)
    {
      pyramid += i * i;
      if(i == 5)
        break;
    }
    print(pyramid, '\n');
  }
  {
    //none
    pyramid: int = 0;
    i: int = 1;
    for(;;)
    {
      pyramid += i * i;
      if(i == 5)
        break;
      i++;
    }
    print(pyramid, '\n');
  }
  {
    //while
    pyramid: int = 0;
    i: int = 1;
    while(i <= 5)
    {
      pyramid += i * i;
      i++;
    }
    print(pyramid, '\n');
  }
}
