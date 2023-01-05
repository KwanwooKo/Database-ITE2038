SELECT trainer.name, SUM(caughtpokemon.level)
FROM trainer, caughtpokemon, (
        SELECT DISTINCT pokemon.id
        FROM pokemon, evolution
        WHERE pokemon.id = evolution.before_id
        OR pokemon.id = evolution.after_id) AS combinedev(id)
WHERE trainer.hometown = 'Blue City'
AND caughtpokemon.owner_id = trainer.id
AND caughtpokemon.pid = combinedev.id
GROUP BY trainer.name
ORDER BY trainer.name;