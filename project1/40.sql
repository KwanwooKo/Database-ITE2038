SELECT trainer.name, city.description
FROM trainer
JOIN caughtpokemon ON caughtpokemon.owner_id = trainer.id
JOIN gym ON gym.leader_id = trainer.id
JOIN city ON city.name = trainer.hometown
JOIN pokemon ON pokemon.id = caughtpokemon.pid
WHERE pokemon.id = ANY(
        SELECT DISTINCT pokemon.id
        FROM pokemon, evolution
        WHERE pokemon.id = evolution.after_id
        ORDER BY pokemon.id)
AND (
        pokemon.type = 'Fire'
        OR pokemon.type = 'Water'
        OR pokemon.type = 'Grass')
GROUP BY trainer.name, city.description
ORDER BY trainer.name;