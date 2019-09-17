proc void main()
{
  (int | char) a = 'a';
  (int | char) b = 532;
  assert(a is char);
  assert(b is int);
  assert(!a is int);
  assert(!b is char);
  (int | char | double) c = b;
  assert(c is int);
  print(a as char, '\n');
  print(b as int, '\n');
  print(c as int, '\n');
}
