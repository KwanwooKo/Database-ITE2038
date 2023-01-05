SELECT SUM(caughtpokemon.level)
FROM caughtpokemon
JOIN pokemon ON pokemon.id = caughtpokemon.pid
WHERE pokemon.type <> 'Fire'
AND pokemon.type <> 'Grass'
AND pokemon.type <> 'Water'
AND pokemon.type <> 'Electric';