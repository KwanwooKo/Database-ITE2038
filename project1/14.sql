SELECT DISTINCT pokemon.name
FROM pokemon
WHERE pokemon.id <> ALL (
        SELECT DISTINCT caughtpokemon.pid
        FROM caughtpokemon)
ORDER BY pokemon.name;