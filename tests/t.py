# -*- encoding:utf-8 -*-

from villa import Villa

def main():
    db = Villa('test.db', 'n')

    db.push('a', '1')
    db.push('a', '2')
    print db.getlist('a')

    db['apple'] = 'red'
    db['apple2'] = 'red'
    db['lemon'] = 'yellow'
    db['orange'] = 'orange'

    print '*' * 100
    #print db.truncate('app')
    for k, v in db.iter('app'):
        print k, v

    print '*' * 100
    for k, v in db.iteritems():
        print k, v

    print list(db.iterkeys())

    print '*' * 100
    print db.sync()
    print db.rnum()
    print db.info()

if __name__ == '__main__':
    main()
