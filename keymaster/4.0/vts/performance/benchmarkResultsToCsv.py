import json,sys,re

def rreplace(s, old, new, count):
    parts = s.rsplit(old, count)
    return new.join(parts)

with open(sys.argv[1]) as json_file:
    results = json.load(json_file)
columns = [('name','test_name'), ('iterations','sample_count'), ('real_time','mean_time')]
with open(sys.argv[2], 'w') as out:
    for column in columns:
        out.write(column[1] + ',')
    out.write('\r\n')
    for result in results['benchmarks']:
        for column in columns:
            if column[0] == 'name':
                name = result[column[0]]
                name = name.lower()
                name = name.replace('/','_',1)
                name = rreplace(name,'/','_',1)
                if 'label' in result:
                    name += '_' + result['label'].split(':')[1]
                out.write(name+',')
            else:
                out.write(str(result[column[0]])+',')
        out.write('\r\n')

