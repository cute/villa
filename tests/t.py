#-*- encoding:utf-8 -*-

from villa import villa

db = villa.open('test.db', 'n')

db.put('a', '1', villa.VL_DDUP)
db.put('a', '2', villa.VL_DDUP)

print db.getlist('a')

db['apple'] = 'red'
db['apple2'] = 'red'
db['lemon'] = 'yellow'
db['orange'] = 'orange'

print '*' * 100
for k, v in db.iteritems():
    print k, v

print '*' * 100
for k, v in db.iterprefix('app', villa.VL_JFORWARD):
    print k, v

print db.sync()
print db.rnum()
print db.info()

db.close()
