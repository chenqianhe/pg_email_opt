psql \
      -v ON_ERROR_STOP=off \
      --pset pager=off \
      --set COLUMNS=200 \
      -P format=aligned \
      -P columns=200 \
      -P expanded=on \
      -f ./sql/init.sql \
      2> ./expect/init.log

psql \
      -v ON_ERROR_STOP=off \
      --pset pager=off \
      --set COLUMNS=200 \
      -P format=aligned \
      -P columns=200 \
      -P expanded=on \
      -f ./sql/test001-insert-correct.sql \
      > ./expect/test001-insert-correct.out \
      2> ./expect/test001-insert-correct.log


psql \
  -v ON_ERROR_STOP=off \
  --pset pager=off \
  --set COLUMNS=200 \
  -P format=aligned \
  -P columns=200 \
  -P expanded=on \
  -f ./sql/test001-insert-wrong.sql \
  2> ./expect/test001-insert-wrong.log

psql \
      -v ON_ERROR_STOP=off \
      --pset pager=off \
      --set COLUMNS=200 \
      -P format=aligned \
      -P columns=200 \
      -P expanded=on \
      -f ./sql/test002-op.sql \
      > ./expect/test002-op.out \
      2> ./expect/test002-op.log

psql \
      -v ON_ERROR_STOP=off \
      --pset pager=off \
      --set COLUMNS=200 \
      -P format=aligned \
      -P columns=200 \
      -P expanded=on \
      -f ./sql/test003-index.sql \
      > ./expect/test003-index.out \
      2> ./expect/test003-index.log

psql \
      -v ON_ERROR_STOP=off \
      --pset pager=off \
      --set COLUMNS=200 \
      -P format=aligned \
      -P columns=200 \
      -P expanded=on \
      -f ./sql/clean.sql \
      2> ./expect/clean.log
