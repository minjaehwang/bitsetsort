import pandas as pd
import matplotlib.pyplot as plt
import argparse

runs = ['sort_znver3_ryzen5950x', 'sort_broadwell_e5_1650']

parser = argparse.ArgumentParser(description='Generate graph for README.md')
parser.add_argument('--pattern', help='if specified, overrides pattern from CSV')

args = parser.parse_args()

for run in runs:
  df = pd.read_csv(run + '.csv')
  df = df.drop(columns=['bytes_per_second', 'items_per_second', 'label', 'error_occurred', 'error_message', 'iterations', 'real_time', 'time_unit'])
  for k, v in df.iterrows():
    s = v['name'].split('_')
    if s[1] == 'Sort':
      s[1] = 'std::sort'

    df.loc[k, 'name'] = s[1]
    df.loc[k, 'datatype'] = s[2]
    df.loc[k, 'pattern'] = s[3]
    df.loc[k, 'n'] = int(s[4])

  file_prefix = ''
  if args.pattern:
    patterns = [args.pattern]
    file_prefix = args.pattern.lower() + '_'
  else:
    patterns = df['pattern'].unique()
  datatypes = ['uint64', 'pair<uint32, uint32>', 'string']
  sortnames = ['std::sort', 'BitsetSort', 'PdqSort']

  plt.figure()

  fig, axes = plt.subplots(len(patterns), len(datatypes), figsize=(5*len(datatypes), 4*len(patterns)))

  if args.pattern is None:
    fig.tight_layout()
    fig.subplots_adjust(hspace=0.3, left=0.05, top=0.98, bottom=0.02)

  for j in range(len(datatypes)):
    datatype = datatypes[j]
    for i in range(len(patterns)):
      pattern = patterns[i]
      if len(patterns) == 1:
        ax = axes[j]
      else:
        ax = axes[i, j]
      ax.set_xscale('log', base=2)

      for v in sortnames:
        df1 = df[(df['pattern']==pattern) & (df['datatype']==datatype) & (df['name']==v)]
        ax.set_xticks(df1['n'])
        ax.scatter(x = df1['n'], y=df1['cpu_time'], label=v)
      ax.legend()
      ax.set_ylabel('Nanosecond per element')
      ax.set_xlabel('Number of elements')
      ax.set_title(pattern + ' ' + datatype)

  plt.savefig(fname=file_prefix + run + '.svg', format='svg')