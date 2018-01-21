function Account:show()
    print("Account balance =", self:balance())
end

a = Account(100)
b = Account:new(30)

print('a =', a)
print('b =', b)
print('metatable =', getmetatable(a))
print('Account =', Account)
for key, value in pairs(Account) do
    print(key, value)
end

a:show() 
a:deposit(50.30) 
a:show() 
a:withdraw(25.10) 
a:show()

parent = {}

function parent:rob(amount)
    amount = amount or self:balance()
    self:withdraw(amount)
    return amount
end

getmetatable(Account).__index = parent

debug.debug()
