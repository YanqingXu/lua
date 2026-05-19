local account = { balance = 10 }

function account:deposit(amount)
    self.balance = self.balance + amount
    return self.balance
end

function account:withdraw(amount)
    self.balance = self.balance - amount
    return self.balance
end

print("after deposit", account:deposit(7))
print("after withdraw", account:withdraw(3))

