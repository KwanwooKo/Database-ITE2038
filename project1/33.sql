SELECT SUM(caughtpokemon.level)
FROM caughtpokemon, pokemon
WHERE caughtpokemon.pid <> ALL(
        SELECT DISTINCT pokemon.id
        FROM pokemon, evolution
        WHERE pokemon.id = evolution.before_id
        OR pokemon.id = evolution.after_id)
AND pokemon.id = caughtpokemon.pid;