class main
  method main(): bool
    x = 3
    do
      x = x + 1
      if x > 10
        break
      end
      x.print
    end
    y = x - 10
    for y < 10
      if y == 0
        (0.01).print
      elif y == 1
        (0.02).print
      else
        (true).print
      end
      y = y + 1
    end
    ret true
  end
end
