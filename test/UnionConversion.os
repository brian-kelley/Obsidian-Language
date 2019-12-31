proc main: void()
{
  typedef (int | char | double | string) UnionA;
  typedef (char | double | string) UnionB;
  typedef (int | ulong | char) UnionC;
  typedef (int | char) UnionD;
  typedef (char | string) UnionE;
  /*
  {
    //Test widening (implicit) conversions
    //Since D just has integer types, it can always
    //convert to any other union containing an integer type.
    UnionD d = 5;
    assert(d is int);
    UnionA a = d;
    UnionB b = d;
    UnionC c = d;
    assert(a is int);   //int is exact match
    assert(b is char);  //char is first converting match
    assert(c is int);  //exact
  }
  */
  {
    //Test narrowing conversions to another union
    UnionA a = "hello\n";
    /*
    assert(a is string);
    assert((a as string) == "hello\n");
    */
    UnionB b = a as UnionB;
    /*
    assert(b is string);
    assert((b as string) == "hello\n");
    UnionE e = a as UnionE;
    assert(e is string);
    assert((e as string) == "hello\n");
    */
  }
  /*
  {
    //Test narrowing to single type
    UnionA a = 'A';
    assert(a is char);
    char charA = a as char;
    assert(charA == 'A');
    //constant doesn't fit in int, should be ulong
    UnionC bignum = 0xAAAABBBBCCCC;
    assert(bignum is ulong);
    assert((bignum as ulong) == 0xAAAABBBBCCCC);
  }
  */
}

