(SELECT DISTINCT 
        t1.name, t1.hometown
FROM 
        trainer t1,
        trainer t2,
        caughtpokemon cp1,
        caughtpokemon cp2,
        evolution e1,
        evolution e2
WHERE t1.id = cp1.owner_id
AND t2.id = cp2.owner_id
AND t1.id = t2.id
AND cp1.pid <> cp2.pid
AND e1.before_id = cp1.pid
AND e1.after_id = e2.before_id
AND e2.after_id = cp2.pid
UNION
SELECT DISTINCT 
        t1.name, t1.hometown
FROM 
        trainer t1,
        trainer t2,
        caughtpokemon cp1,
        caughtpokemon cp2,
        evolution
WHERE t1.id = cp1.owner_id
AND t2.id = cp2.owner_id
AND t1.id = t2.id
AND cp1.pid <> cp2.pid
AND evolution.before_id = cp1.pid
AND evolution.after_id = cp2.pid)
ORDER BY name;