#-*- encoding:utf-8 -*-

from villa import villa

db = villa.open('test.db', 'n')

db.put('a', '1', villa.VL_DDUP)
db.put('a', '2', villa.VL_DDUP)

print db.getlist('a')

db['apple'] = 'red'
db['lemon'] = 'black'
db['orange'] = 'orange'
db['lemon'] = 'yellow'

print db['lemon']

for k, v in db.iteritems():
    print k, v

db.close()
