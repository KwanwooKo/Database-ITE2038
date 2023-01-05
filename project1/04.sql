SELECT DISTINCT pokemon.name
FROM pokemon, evolution
WHERE evolution.before_id > evolution.after_id
AND pokemon.id = evolution.before_id;