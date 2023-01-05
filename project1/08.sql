SELECT trainer.name, AVG(typepokemon.level)
FROM (
    SELECT trainer.name, caughtpokemon.level, pokemon.name
    FROM trainer, pokemon, caughtpokemon
    WHERE trainer.id = caughtpokemon.owner_id
    AND pokemon.id = caughtpokemon.pid
    AND (pokemon.type = 'Normal' OR pokemon.type = 'Electric')) AS typepokemon(tname, level, pname), trainer, pokemon
WHERE typepokemon.tname = trainer.name
AND pokemon.name = typepokemon.pname
GROUP BY trainer.name
ORDER BY AVG(typepokemon.level), trainer.name;