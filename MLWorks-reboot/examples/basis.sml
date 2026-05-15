val args = CommandLine.arguments ()
val n = case args of [] => 3 | x :: _ => valOf (Int.fromString x)
val xs = [n, n + 1, n + 2]
val _ = print (Int.toString (List.nth (xs, 1)) ^ "\n")
